#pragma once // 추가되어 있는지 확인하세요

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking.h"
#include "Shared.h" // Shared.h도 여기에 포함시켜주세요
#include "MyNetworkActor.generated.h" // 1. 파일명과 동일하게 수정

UCLASS()
class TESTCLI_API AMyNetworkActor : public AActor // 2. 파일명과 클래스명 일치 확인
{
    GENERATED_BODY() // 이제 여기서 오류가 나지 않을 것입니다.

public:
    AMyNetworkActor(); // 생성자 (필요시)

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    FSocket* Socket;
    int32 MyPlayerId = -1;

    void SendMovePacket();
    void ReceivePackets();
};
