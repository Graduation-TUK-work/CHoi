#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyRoleSelectWidget.generated.h"

class UButton;

UCLASS()
class MYPROJECT_START_API ULobbyRoleSelectWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* KillerButton;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* RunnerButton;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* Killer_BT;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* Runner_BT;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* KILLER;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* RUNNER;

private:
    UFUNCTION()
    void HandleKillerClicked();

    UFUNCTION()
    void HandleRunnerClicked();
};
