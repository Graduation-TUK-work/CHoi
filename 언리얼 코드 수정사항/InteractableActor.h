#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionInterface.h"
#include "InteractableActor.generated.h"

UCLASS()
class MYPROJECT_START_API AInteractableActor : public AActor, public IInteractionInterface
{
    GENERATED_BODY()

public:
    AInteractableActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    float RepairDuration = 80.f;

    UPROPERTY(VisibleAnywhere, Category = "Interaction")
    float RepairProgress = 0.f;

    UPROPERTY(VisibleAnywhere, Category = "Interaction")
    bool bIsRepaired = false;

    bool bIsBeingRepaired = false;

public:
    // 1. 인터페이스에 선언된 파라미터 타입(ACharacter*)이 정확한지 확인
    // 2. const 위치가 인터페이스와 동일한지 확인

    virtual void StartInteract_Implementation(class ACharacter* Interactor) override;

    virtual void UpdateInteract_Implementation(float DeltaTime) override;

    virtual void CancelInteract_Implementation() override;

    virtual void CompleteInteract_Implementation() override;

    // 인터페이스에서 const로 선언했다면 여기서도 반드시 const여야 함
    virtual float GetInteractDuration_Implementation() const override;
};