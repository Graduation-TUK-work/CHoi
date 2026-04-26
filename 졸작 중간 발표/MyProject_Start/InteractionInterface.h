#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractionInterface : public UInterface
{
    GENERATED_BODY()
};

class MYPROJECT_START_API IInteractionInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent)
    void StartInteract(ACharacter* Interactor);
    virtual void StartInteract_Implementation(ACharacter* Interactor) {}

    UFUNCTION(BlueprintNativeEvent)
    void UpdateInteract(float DeltaTime);
    virtual void UpdateInteract_Implementation(float DeltaTime) {}

    UFUNCTION(BlueprintNativeEvent)
    void CancelInteract();
    virtual void CancelInteract_Implementation() {}

    UFUNCTION(BlueprintNativeEvent)
    void CompleteInteract();
    virtual void CompleteInteract_Implementation() {}

    // 진행도 없는 오브젝트는 0.f 반환
    UFUNCTION(BlueprintNativeEvent)
    float GetInteractDuration() const;
    virtual float GetInteractDuration_Implementation() const { return 0.f; }
};