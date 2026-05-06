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
    // BlueprintCallable을 추가하면 블루프린트에서 이 인터페이스 함수를 직접 호출하거나
    // 더 명확하게 인터페이스 존재 여부를 파악할 수 있습니다.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void StartInteract(ACharacter* Interactor);
    virtual void StartInteract_Implementation(ACharacter* Interactor) {}

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void UpdateInteract(float DeltaTime);
    virtual void UpdateInteract_Implementation(float DeltaTime) {}

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void CancelInteract();
    virtual void CancelInteract_Implementation() {}

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void CompleteInteract();
    virtual void CompleteInteract_Implementation() {}

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    float GetInteractDuration() const;
    virtual float GetInteractDuration_Implementation() const { return 0.f; }
};