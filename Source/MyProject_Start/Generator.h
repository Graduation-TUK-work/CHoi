#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject_Start/InteractionInterface.h"
#include "Generator.generated.h"

UCLASS()
class MYPROJECT_START_API AGenerator : public AActor, public IInteractionInterface
{
    GENERATED_BODY()

public:
    AGenerator();

    // IInteractionInterface
    virtual void StartInteract_Implementation(ACharacter* Interactor) override;
    virtual void UpdateInteract_Implementation(float DeltaTime) override;
    virtual void CancelInteract_Implementation() override;
    virtual void CompleteInteract_Implementation() override;
    virtual float GetInteractDuration_Implementation() const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generator")
    int32 GeneratorId = -1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generator")
    bool bIsRepaired = false;

    UFUNCTION(BlueprintCallable, Category = "Generator")
    int32 GetGeneratorId() const;

    UFUNCTION(BlueprintCallable, Category = "Generator")
    float GetRepairProgress() const;

    UFUNCTION(BlueprintCallable, Category = "Generator")
    bool IsBeingRepaired() const;

    void ApplyNetworkRepairState(bool bInBeingRepaired, bool bInRepaired, float InRepairProgress);

    UFUNCTION(BlueprintImplementableEvent, Category = "Generator")
    void OnRepairStateChanged(bool bInBeingRepaired, bool bInRepaired, float InRepairProgress);

    UFUNCTION(BlueprintImplementableEvent, Category = "Generator")
    void OnRepairCompleted();

protected:
    virtual void BeginPlay() override;
    UPROPERTY()
    class ACharacter* CurrentInteractor;
private:
    // 수리 진행도 (0.0 ~ 1.0)
    UPROPERTY(VisibleAnywhere, Category = "Generator")
    float RepairProgress = 0.f;

    // 총 수리 시간
    UPROPERTY(EditAnywhere, Category = "Generator")
    float RepairDuration = 80.f;

    // 현재 수리 중인지
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generator", meta = (AllowPrivateAccess = "true"))
    bool bIsBeingRepaired = false;

    void AssignGeneratedIdIfNeeded();
    void SetRepairState(bool bInBeingRepaired, bool bInRepaired, float InRepairProgress);
};