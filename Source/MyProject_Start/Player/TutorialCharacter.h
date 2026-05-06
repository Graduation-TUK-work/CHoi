#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MotionWarpingComponent.h"
#include "MyProject_Start/InteractionInterface.h"
#include "TutorialCharacter.generated.h"

// ★ 전방 선언은 UCLASS() 위쪽으로 옮겨야 합니다.
class FNetworkWorker;
class AKillerCharacter;
class AGenerator;
class UAnimSequence;

UCLASS() // UCLASS와 class ATutorialCharacter 사이에는 아무것도 없어야 합니다.
class MYPROJECT_START_API ATutorialCharacter : public ACharacter, public IInteractionInterface
{
	GENERATED_BODY()

public:
	// 상호작용 인터페이스 구현
	virtual void StartInteract_Implementation(ACharacter* Interactor) override;
	virtual void UpdateInteract_Implementation(float DeltaTime) override;
	virtual void CancelInteract_Implementation() override;
	virtual void CompleteInteract_Implementation() override;

	// 회복 관련 변수
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float RecoveryProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxRecoveryTime = 4.0f;

	// 블루프린트에서 호출할 수 있도록 Callable 추가
	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void ForceDownedState();

	ATutorialCharacter();
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	void BeginSprint();
	void EndSprint();
	void BeginCrouch();
	void EndCrouch();

	TMap<int32, ATutorialCharacter*> RemotePlayers;
	TMap<int32, AKillerCharacter*> RemoteKillers;

	UPROPERTY()
	AActor* CurrentInteractable;

	void StartInteraction();
	void CancelInteraction();
	void TraceForInteractable();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* HitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* DownedMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* HitReactionAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* DownedAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* RepairAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool IsDowned = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool IsBeingCarried = false;

	void UpdateRemotePlayer(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint);
	void UpdateRemoteKiller(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint);
	void HandleNetworkAction(uint8 ActionType, int32 InstigatorId, int32 TargetId, FVector Location, float RotationYaw);
	void SendGeneratorActionToServer(uint8 ActionType, AGenerator* Generator);
	void SetRepairingGenerator(bool bRepairing);

	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	float RemoteForwardValue;
	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	float RemoteRightValue;
	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	bool RemoteIsSprinting;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	int32 CurrentHealth = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bCanBeHit = true;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetCanVault(bool CanIt) { bCanVault = CanIt; }

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY()
	float MoveForwardValue = 0;
	UPROPERTY()
	float MoveRightValue = 0;

	UPROPERTY(BlueprintReadOnly)
	float AimYaw = 0;
	UPROPERTY(BlueprintReadOnly)
	float AimPitch = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool IsInteracting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bIsRepairingGenerator = false;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayHitReaction();
	void PlayNetworkHitReaction();

	FNetworkWorker* NetworkWorker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network")
	int32 MyPlayerId = -1;

	void SendLocationToServer();

protected:
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* SpringArm;
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	bool bCanVault;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	UAnimMontage* VaultMontage;

	UPROPERTY(VisibleAnywhere)
	UMotionWarpingComponent* MotionWarping;

	void TryVault();
	bool bIsVaulting = false;
	FVector VaultStartLocation;
	FVector VaultTargetLocation;

	bool bIsVaultMoving = false;
	float VaultAlpha = 0.0f;

private:
	void ShowGeneratorRepairCount() const;
	AGenerator* FindGeneratorForNetworkAction(int32 GeneratorId, const FVector& Location) const;
	void ApplyGeneratorNetworkAction(uint8 ActionType, int32 GeneratorId, const FVector& Location, float RepairProgress);
	void ApplyHitReaction(bool bRespectCooldown);
	void PlayTemporaryBodyAnimation(UAnimSequence* Animation);
	void PlayLoopBodyAnimation(UAnimSequence* Animation);
	void RestoreBodyAnimClass();
};