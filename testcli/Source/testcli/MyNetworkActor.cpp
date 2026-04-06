#include "MyNetworkActor.h"
#include "Shared.h"

AMyNetworkActor::AMyNetworkActor()
{
    // 중요: 이 설정이 있어야 Tick() 함수가 매 프레임 호출됩니다.
    PrimaryActorTick.bCanEverTick = true;

    // 소켓 변수 초기화
    Socket = nullptr;
}
void AMyNetworkActor::BeginPlay()
{
    Super::BeginPlay();

    // 1. 소켓 생성 및 서버 연결
    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("Default"), false);

    FIPv4Address IP;
    FIPv4Address::Parse(TEXT("127.0.0.1"), IP);
    TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    Addr->SetIp(IP.Value);
    Addr->SetPort(7777);

    if (Socket->Connect(*Addr)) {
        UE_LOG(LogTemp, Warning, TEXT("서버 연결 성공!"));
    }
}

void AMyNetworkActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 수신 루프
    ReceivePackets();

    // 테스트용: 1초마다 내 좌표 전송 (실제로는 입력 발생 시 전송)
    static float Timer = 0;
    Timer += DeltaTime;
    if (Timer > 1.0f) {
        SendMovePacket();
        Timer = 0;
    }
}

void AMyNetworkActor::SendMovePacket()
{
    if (!Socket || MyPlayerId == -1) return;

    FPacketMove MovePkt;
    MovePkt.Type = PKT_MOVE;
    MovePkt.Data.PlayerId = MyPlayerId;

    FVector Loc = GetActorLocation();
    MovePkt.Data.X = Loc.X;
    MovePkt.Data.Y = Loc.Y;
    MovePkt.Data.Z = Loc.Z;

    int32 BytesSent = 0;
    Socket->Send((uint8*)&MovePkt, sizeof(MovePkt), BytesSent);
}

void AMyNetworkActor::ReceivePackets()
{
    uint32 PendingDataSize = 0;
    while (Socket->HasPendingData(PendingDataSize)) {
        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(PendingDataSize);

        int32 BytesRead = 0;
        if (Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead)) {
            uint8 PacketType = ReceivedData[0];

            if (PacketType == PKT_JOIN) {
                FPacketJoin* JoinPkt = (FPacketJoin*)ReceivedData.GetData();
                MyPlayerId = JoinPkt->MyId;
                UE_LOG(LogTemp, Warning, TEXT("내 접속 ID: %d"), MyPlayerId);
            }
            else if (PacketType == PKT_MOVE) {
                FPacketMove* MovePkt = (FPacketMove*)ReceivedData.GetData();
                // 여기서 다른 플레이어의 위치를 업데이트하는 로직 구현
                UE_LOG(LogTemp, Log, TEXT("다른 유저[%d] 이동: %f, %f, %f"),
                    MovePkt->Data.PlayerId, MovePkt->Data.X, MovePkt->Data.Y, MovePkt->Data.Z);
            }
        }
    }
}

void AMyNetworkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (Socket) {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
    }
}