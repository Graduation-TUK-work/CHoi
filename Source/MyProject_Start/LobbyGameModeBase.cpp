#include "LobbyGameModeBase.h"

#include "LobbyPlayerController.h"

ALobbyGameModeBase::ALobbyGameModeBase()
{
    PlayerControllerClass = ALobbyPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
}
