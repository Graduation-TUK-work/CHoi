#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "GameFramework/Character.h"
#include "MotionWarpingComponent.h"
#include "MyProject_Start/InteractionInterface.h"
#include "TutorialCharacter.generated.h"

class FNetworkWorker;
class AKillerCharacter;
class AGenerator;
class ABandagePickup;
class AParkourInteractable;
class UAnimSequence;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;
class SImage;
class STextBlock;
class SProgressBar;
class SWidget;

UCLASS()
class MYPROJECT_START_API ATutorialCharacter : public ACharacter, public IInteractionInterface
{
	GENERATED_BODY()

public:
	virtual void StartInteract_Implementation(ACharacter* Interactor) override;
	virtual void UpdateInteract_Implementation(float DeltaTime) override;
	virtual void CancelInteract_Implementation() override;
	virtual void CompleteInteract_Implementation() override;
	virtual float GetInteractDuration_Implementation() const override;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float RecoveryProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxRecoveryTime = 5.33f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float SelfRecoveryTime = 8.0f;

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void ForceDownedState();

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void ForceInjuredState();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* RepairMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool IsDowned = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool IsBeingCarried = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsBeingRevived = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsSelfReviving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bHasBandage = false;

	void UpdateRemotePlayer(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint, int32 InHealth, bool bInDowned, bool bInBeingCarried);
	void UpdateRemoteKiller(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint);
	void HandleNetworkAction(uint8 ActionType, int32 InstigatorId, int32 TargetId, FVector Location, float RotationYaw);
	void SendGeneratorActionToServer(uint8 ActionType, AGenerator* Generator);
	void SendBandageActionToServer(uint8 ActionType, ABandagePickup* BandagePickup);
	void SetRepairingGenerator(bool bRepairing);
	void SetRevivingSurvivor(bool bReviving);
	void StopInteractingWithCurrentTarget(bool bRestoreAnimation);
	void StartVaultFromInteractable(AParkourInteractable* ParkourInteractable);
	bool HasBandage() const { return bHasBandage; }
	void SetHasBandage(bool bNewHasBandage) { bHasBandage = bNewHasBandage; }

	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	float RemoteForwardValue;
	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	float RemoteRightValue;
	UPROPERTY(BlueprintReadOnly, Category = "NetworkData")
	bool RemoteIsSprinting;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	int32 CurrentHealth = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bCanBeHit = true;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Status")
	bool IsInjured() const { return CurrentHealth == 1 && !IsDowned && !IsBeingCarried; }

	UFUNCTION(BlueprintPure, Category = "Status")
	int32 GetCurrentHealth() const { return CurrentHealth; }

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
	void SyncRemoteState(int32 InHealth, bool bInDowned, bool bInBeingCarried);

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
	void ShowGeneratorRepairWidget(AGenerator* Generator);
	void UpdateGeneratorRepairWidget(AGenerator* Generator);
	void HideGeneratorRepairWidget();
    void ShowReviveProgressWidget();
    void UpdateReviveProgressWidget(float Progress);
    void HideReviveProgressWidget();
	AGenerator* FindGeneratorForNetworkAction(int32 GeneratorId, const FVector& Location) const;
	ABandagePickup* FindBandageForNetworkAction(int32 BandageId, const FVector& Location) const;
	void ApplyGeneratorNetworkAction(uint8 ActionType, int32 GeneratorId, const FVector& Location, float RepairProgress);
	void ApplyBandageNetworkAction(int32 BandageId, const FVector& Location);
	void SendSurvivorActionToServer(uint8 ActionType, ATutorialCharacter* TargetCharacter);
	void ApplyHitReaction(bool bRespectCooldown);
	void PlayTemporaryBodyAnimation(UAnimSequence* Animation);
	void PlayLoopBodyAnimation(UAnimSequence* Animation);
	void PlayRepairMontageStart();
	void PlayRepairMontageEnd();
	void RestoreBodyAnimClass();
	void ApplyLocalPlayerInputMode();
	bool CanStartVault() const;
	void BeginVaultToLocation(const FVector& TargetLocation);
	void FinishVault();

	TSharedPtr<SWidget> GeneratorRepairWidget;
	TSharedPtr<SImage> GeneratorRepairProgressImage1;
	TSharedPtr<SImage> GeneratorRepairProgressImage2;
	TSharedPtr<SImage> GeneratorRepairProgressImage3;
	TSharedPtr<STextBlock> GeneratorRepairText;
    TSharedPtr<SWidget> ReviveProgressWidget;
    TSharedPtr<SProgressBar> ReviveProgressBar;
    TSharedPtr<STextBlock> ReviveProgressText;
	FSlateBrush GeneratorRepairBackgroundBrush;
	FSlateBrush GeneratorRepairFillBrush1;
	FSlateBrush GeneratorRepairFillBrush2;
	FSlateBrush GeneratorRepairFillBrush3;
	float GeneratorRepairProgressValues[3] = { 0.f, 0.f, 0.f };
	float GeneratorRepairSyncAccumulator = 0.f;
	float ReviveSyncAccumulator = 0.f;

	UPROPERTY(Transient)
	UTexture2D* GeneratorRepairBackgroundTexture = nullptr;

	UPROPERTY(Transient)
	UMaterialInterface* GeneratorRepairFillMaterial = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* GeneratorRepairFillMID1 = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* GeneratorRepairFillMID2 = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* GeneratorRepairFillMID3 = nullptr;

	TWeakObjectPtr<ATutorialCharacter> CurrentReviver;
	FTimerHandle VaultFinishTimerHandle;
};
