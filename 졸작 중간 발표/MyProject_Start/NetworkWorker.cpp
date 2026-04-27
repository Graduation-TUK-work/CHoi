#include "NetworkWorker.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "MyProject_Start/Player/TutorialCharacter.h"
#include "MyProject_Start/KillerCharacter.h"

FNetworkWorker::FNetworkWorker(FString IP, int32 Port)
{
    ServerIP = IP;
    ServerPort = Port;
    bRunning = true;
    Socket = nullptr;
    OwnerTutorialCharacter = nullptr;
    OwnerKillerCharacter = nullptr;

    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("Default"), false);
}

bool FNetworkWorker::Init()
{
    FIPv4Address Addr;
    FIPv4Address::Parse(ServerIP, Addr);
    TSharedRef<FInternetAddr> TAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    TAddr->SetIp(Addr.Value);
    TAddr->SetPort(ServerPort);

    if (Socket->Connect(*TAddr))
    {
        UE_LOG(LogTemp, Warning, TEXT("서버에 연결되었습니다!"));
        return true;
    }
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

                    AsyncTask(ENamedThreads::GameThread, [this, AssignedId = JoinPkt->MyId]()
                        {
                            if (IsValid(OwnerTutorialCharacter))
                            {
                                OwnerTutorialCharacter->MyPlayerId = AssignedId;
                                UE_LOG(LogTemp, Warning, TEXT("나의 플레이어 ID 할당됨: %d"), AssignedId);
                            }
                            else if (IsValid(OwnerKillerCharacter))
                            {
                                OwnerKillerCharacter->MyPlayerId = AssignedId;
                                UE_LOG(LogTemp, Warning, TEXT("나의 킬러 ID 할당됨: %d"), AssignedId);
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
