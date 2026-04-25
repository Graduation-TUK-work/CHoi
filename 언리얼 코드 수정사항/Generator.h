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

protected:
    virtual void BeginPlay() override;

private:
    // 수리 진행도 (0.0 ~ 1.0)
    UPROPERTY(VisibleAnywhere, Category = "Generator")
    float RepairProgress = 0.f;

    // 총 수리 시간
    UPROPERTY(EditAnywhere, Category = "Generator")
    float RepairDuration = 80.f;

    // 수리 완료 여부
    UPROPERTY(VisibleAnywhere, Category = "Generator")
    bool bIsRepaired = false;

    // 현재 수리 중인지
    bool bIsBeingRepaired = false;
};