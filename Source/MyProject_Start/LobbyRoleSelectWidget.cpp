#include "LobbyRoleSelectWidget.h"

#include "Components/Button.h"
#include "MyGameInstance.h"

void ULobbyRoleSelectWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (KillerButton)
    {
        KillerButton->OnClicked.AddDynamic(this, &ULobbyRoleSelectWidget::HandleKillerClicked);
    }
    if (Killer_BT)
    {
        Killer_BT->OnClicked.AddDynamic(this, &ULobbyRoleSelectWidget::HandleKillerClicked);
    }
    if (KILLER)
    {
        KILLER->OnClicked.AddDynamic(this, &ULobbyRoleSelectWidget::HandleKillerClicked);
    }
    if (RunnerButton)
    {
        RunnerButton->OnClicked.AddDynamic(this, &ULobbyRoleSelectWidget::HandleRunnerClicked);
    }
    if (Runner_BT)
    {
        Runner_BT->OnClicked.AddDynamic(this, &ULobbyRoleSelectWidget::HandleRunnerClicked);
    }
    if (RUNNER)
    {
        RUNNER->OnClicked.AddDynamic(this, &ULobbyRoleSelectWidget::HandleRunnerClicked);
    }
}

void ULobbyRoleSelectWidget::HandleKillerClicked()
{
    if (UMyGameInstance* GameInstance = GetGameInstance<UMyGameInstance>())
    {
        GameInstance->SelectKillerAndOpenGame();
    }
}

void ULobbyRoleSelectWidget::HandleRunnerClicked()
{
    if (UMyGameInstance* GameInstance = GetGameInstance<UMyGameInstance>())
    {
        GameInstance->SelectRunnerAndOpenGame();
    }
}
