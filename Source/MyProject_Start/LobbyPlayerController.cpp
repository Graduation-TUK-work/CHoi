#include "LobbyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

ALobbyPlayerController::ALobbyPlayerController()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> LobbyWidgetFinder(TEXT("/Game/1_Map/WBP_Lobby"));
    if (LobbyWidgetFinder.Succeeded())
    {
        LobbyWidgetClass = LobbyWidgetFinder.Class;
    }
}

void ALobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    bShowMouseCursor = true;

    FInputModeUIOnly InputMode;
    SetInputMode(InputMode);

    if (LobbyWidgetClass)
    {
        LobbyWidget = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
        if (LobbyWidget)
        {
            LobbyWidget->AddToViewport();
        }
    }
}
