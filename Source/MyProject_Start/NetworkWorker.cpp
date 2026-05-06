#include "NetworkWorker.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "MyProject_Start/Player/TutorialCharacter.h"
#include "MyProject_Start/KillerCharacter.h"

namespace
{
    constexpr TCHAR NetworkConfigSection[] = TEXT("/Script/MyProject_Start.NetworkSettings");
    constexpr TCHAR ServerIpKey[] = TEXT("ServerIP");
    constexpr TCHAR ServerPortKey[] = TEXT("ServerPort");
    constexpr TCHAR DefaultServerIp[] = TEXT("127.0.0.1");
    constexpr int32 DefaultServerPort = 7777;
}

FNetworkWorker::FNetworkWorker(FString IP, int32 Port)
{
    ServerIP = IP;
    ServerPort = Port;
    bRunning = true;
    Socket = nullptr;
    OwnerTutorialCharacter = nullptr;
    OwnerKillerCharacter = nullptr;
    PendingSelectedRole = ROLE_NONE;

    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("Default"), false);
}

FString FNetworkWorker::GetDefaultServerIP()
{
    FString ConfiguredIP = DefaultServerIp;
    if (GConfig)
    {
        GConfig->GetString(NetworkConfigSection, ServerIpKey, ConfiguredIP, GGameIni);
    }

    ConfiguredIP.TrimStartAndEndInline();
    return ConfiguredIP.IsEmpty() ? FString(DefaultServerIp) : ConfiguredIP;
}

int32 FNetworkWorker::GetDefaultServerPort()
{
    int32 ConfiguredPort = DefaultServerPort;
    if (GConfig)
    {
        GConfig->GetInt(NetworkConfigSection, ServerPortKey, ConfiguredPort, GGameIni);
    }

    return ConfiguredPort > 0 ? ConfiguredPort : DefaultServerPort;
}

bool FNetworkWorker::Init()
{
    FIPv4Address Addr;
    if (!FIPv4Address::Parse(ServerIP, Addr))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid server IP: %s"), *ServerIP);
        return false;
    }

    TSharedRef<FInternetAddr> TAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    TAddr->SetIp(Addr.Value);
    TAddr->SetPort(ServerPort);

    UE_LOG(LogTemp, Warning, TEXT("Trying server connection: %s:%d"), *ServerIP, ServerPort);

    if (Socket->Connect(*TAddr))
    {
        UE_LOG(LogTemp, Warning, TEXT("Connected to server: %s:%d"), *ServerIP, ServerPort);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to connect to server: %s:%d"), *ServerIP, ServerPort);
    return false;
}

uint32 FNetworkWorker::Run()
{
    while (bRunning)
    {
        if (!Socket || !bRunning) break;

        uint8 Buffer[4096];
        int32 BytesRead = 0;

        if (Socket->Recv(Buffer, sizeof(Buffer), BytesRead))
        {
            if (!bRunning || BytesRead <= 0) continue;

            int32 Offset = 0;
            while (Offset < BytesRead)
            {
                const uint8 PacketType = Buffer[Offset];
                int32 PacketSize = 0;

                if (PacketType == PKT_JOIN)
                {
                    PacketSize = sizeof(FPacketJoin);
                }
                else if (PacketType == PKT_MOVE)
                {
                    PacketSize = sizeof(FPacketMove);
                }
                else if (PacketType == PKT_ACTION)
                {
                    PacketSize = sizeof(FPacketAction);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Unknown packet type: %d"), PacketType);
                    break;
                }

                if (Offset + PacketSize > BytesRead)
                {
                    break;
                }

                uint8* PacketData = Buffer + Offset;

                if (PacketType == PKT_JOIN)
                {
                    FPacketJoin* JoinPkt = (FPacketJoin*)PacketData;
                    this->CachedMyPlayerId = JoinPkt->MyId;
                    if (PendingSelectedRole != ROLE_NONE)
                    {
                        SendRolePacket(PendingSelectedRole);
                    }

                    AsyncTask(ENamedThreads::GameThread, [this, AssignedId = JoinPkt->MyId]()
                        {
                            if (IsValid(OwnerTutorialCharacter))
                            {
                                OwnerTutorialCharacter->MyPlayerId = AssignedId;
                                UE_LOG(LogTemp, Warning, TEXT("Assigned runner ID: %d"), AssignedId);
                            }
                            else if (IsValid(OwnerKillerCharacter))
                            {
                                OwnerKillerCharacter->MyPlayerId = AssignedId;
                                UE_LOG(LogTemp, Warning, TEXT("Assigned killer ID: %d"), AssignedId);
                            }
                        });
                }
                else if (PacketType == PKT_MOVE)
                {
                    FPacketMove* MovePkt = (FPacketMove*)PacketData;
                    int32 RemoteId = MovePkt->Data.PlayerId;

                    if (RemoteId != this->CachedMyPlayerId)
                    {
                        uint8 CharacterType = MovePkt->Data.CharacterType;
                        FVector NewLoc(MovePkt->Data.X, MovePkt->Data.Y, MovePkt->Data.Z);
                        float Yaw = MovePkt->Data.RotationYaw;
                        float Fwd = MovePkt->Data.ForwardValue;
                        float Rght = MovePkt->Data.RightValue;
                        bool bSpr = MovePkt->Data.bIsSprinting;

                        AsyncTask(ENamedThreads::GameThread, [this, RemoteId, CharacterType, NewLoc, Yaw, Fwd, Rght, bSpr]()
                            {
                                if (IsValid(OwnerTutorialCharacter) && !OwnerTutorialCharacter->IsPendingKillPending())
                                {
                                    if (CharacterType == CHARACTER_KILLER)
                                    {
                                        OwnerTutorialCharacter->UpdateRemoteKiller(RemoteId, NewLoc, Yaw, Fwd, Rght, bSpr);
                                    }
                                    else
                                    {
                                        OwnerTutorialCharacter->UpdateRemotePlayer(RemoteId, NewLoc, Yaw, Fwd, Rght, bSpr);
                                    }
                                }
                                else if (IsValid(OwnerKillerCharacter) && !OwnerKillerCharacter->IsPendingKillPending())
                                {
                                    if (CharacterType == CHARACTER_KILLER)
                                    {
                                        OwnerKillerCharacter->UpdateRemoteKiller(RemoteId, NewLoc, Yaw, Fwd, Rght, bSpr);
                                    }
                                    else
                                    {
                                        OwnerKillerCharacter->UpdateRemoteSurvivor(RemoteId, NewLoc, Yaw, Fwd, Rght, bSpr);
                                    }
                                }
                            });
                    }
                }
                else if (PacketType == PKT_ACTION)
                {
                    FPacketAction* ActionPkt = (FPacketAction*)PacketData;

                    if (ActionPkt->InstigatorId != this->CachedMyPlayerId)
                    {
                        uint8 ActionType = ActionPkt->ActionType;
                        int32 InstigatorId = ActionPkt->InstigatorId;
                        int32 TargetId = ActionPkt->TargetId;
                        FVector ActionLocation(ActionPkt->X, ActionPkt->Y, ActionPkt->Z);
                        float ActionYaw = ActionPkt->RotationYaw;

                        AsyncTask(ENamedThreads::GameThread, [this, ActionType, InstigatorId, TargetId, ActionLocation, ActionYaw]()
                            {
                                if (IsValid(OwnerTutorialCharacter) && !OwnerTutorialCharacter->IsPendingKillPending())
                                {
                                    OwnerTutorialCharacter->HandleNetworkAction(ActionType, InstigatorId, TargetId, ActionLocation, ActionYaw);
                                }
                                else if (IsValid(OwnerKillerCharacter) && !OwnerKillerCharacter->IsPendingKillPending())
                                {
                                    OwnerKillerCharacter->HandleNetworkAction(ActionType, InstigatorId, TargetId, ActionLocation, ActionYaw);
                                }
                            });
                    }
                }

                Offset += PacketSize;
            }
        }

        FPlatformProcess::Sleep(0.01f);
    }
    return 0;
}


void FNetworkWorker::SendRolePacket(uint8 InRole)
{
    PendingSelectedRole = InRole;

    if (!Socket || CachedMyPlayerId < 0)
    {
        return;
    }

    FPacketRoleSelect RolePkt;
    RolePkt.Type = PKT_ROLE_SELECT;
    RolePkt.PlayerId = CachedMyPlayerId;
    RolePkt.Role = InRole;

    int32 BytesSent = 0;
    Socket->Send((uint8*)&RolePkt, sizeof(FPacketRoleSelect), BytesSent);
}
void FNetworkWorker::Stop()
{
    bRunning = false;
}

FNetworkWorker::~FNetworkWorker()
{
    Stop();

    if (Socket)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }
}

