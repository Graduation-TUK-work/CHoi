// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MotionWarpingComponent.h"
#include "MyProject_Start/InteractionInterface.h"
#include "TutorialCharacter.generated.h"

// 1. 전방 선언: 클래스 포인터를 사용하기 위해 선언합니다.
class FNetworkWorker;
class AKillerCharacter;
class UAnimSequence;

UCLASS()
class MYPROJECT_START_API ATutorialCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATutorialCharacter();
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	// 달리기, 앉기
	void BeginSprint();
	void EndSprint();
	void BeginCrouch();
	void EndCrouch();

	// 플레이어 ID 관리 맵
	TMap<int32, ATutorialCharacter*> RemotePlayers;
    TMap<int32, AKillerCharacter*> RemoteKillers;


	// 상호작용
	UPROPERTY()
	AActor* CurrentInteractable;

	// 함수
	void StartInteraction();
	void CancelInteraction();
	void TraceForInteractable();

	// 눕는 애니메이션 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* HitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* DownedMontage;

	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimSequence* HitReactionAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimSequence* DownedAnimation;
// 빈사 상태인지 확인하는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool IsDowned = false;

	// 살인마에게 들려있는 상태인지 확인하는 함수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool IsBeingCarried = false;

	// 다른 플레이어의 위치 정보를 갱신하거나 새로 생성하는 함수
	void UpdateRemotePlayer(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint);
    void UpdateRemoteKiller(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint);
    void HandleNetworkAction(uint8 ActionType, int32 InstigatorId, int32 TargetId, FVector Location, float RotationYaw);

	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	float RemoteForwardValue;
	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	float RemoteRightValue;
	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	bool RemoteIsSprinting;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// 2. 소멸자 추가: 스레드를 안전하게 종료하기 위함
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 블루 프린트 권한 설정
	// 현재 체력 (기본값 2: 한 번 맞으면 부상, 두 번 맞으면 빈사)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	int32 CurrentHealth = 2;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bCanBeHit = true; // 처음엔 맞을 수 있어야 하므로 true

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// 블루프린트에서 호출할 함수(파쿠르)
	UFUNCTION(BlueprintCallable)
	void SetCanVault(bool CanIt) { bCanVault = CanIt; }

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	UPROPERTY()
	float MoveForwardValue = 0;
	UPROPERTY()
	float MoveRightValue = 0;
	// 카메라 머리 방향 회전 변수
	UPROPERTY(BlueprintReadOnly)
	float AimYaw = 0;
	UPROPERTY(BlueprintReadOnly)
	float AimPitch = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool IsInteracting = false;

	// 살인마의 CheckHit에서 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayHitReaction();
	void PlayNetworkHitReaction();
	void ForceDownedState();

	// ------------------------------------------------------------
	// 3. 서버 관련 멤버 변수
	// ------------------------------------------------------------
	// 네트워크 통신을 담당하는 워커 스레드 객체
	FNetworkWorker* NetworkWorker;

	// 서버로부터 할당받은 고유 ID (PKT_JOIN에서 받음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network")
	int32 MyPlayerId = -1;

	// 서버로 내 위치를 전송하는 함수
	void SendLocationToServer();
	// ------------------------------------------------------------

protected:
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* SpringArm;
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;
	// 파쿠르 관련 변수, 함수
	bool bCanVault;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	UAnimMontage* VaultMontage;

	UPROPERTY(VisibleAnywhere)
	UMotionWarpingComponent* MotionWarping;

	// 파쿠르
	void TryVault();
	bool bIsVaulting = false;
	FVector VaultStartLocation;
	FVector VaultTargetLocation;

	bool bIsVaultMoving = false;
	float VaultAlpha = 0.0f;

private:
    void ApplyHitReaction(bool bRespectCooldown);
    void PlayTemporaryBodyAnimation(UAnimSequence* Animation);
    void PlayLoopBodyAnimation(UAnimSequence* Animation);
    void RestoreBodyAnimClass();

};
