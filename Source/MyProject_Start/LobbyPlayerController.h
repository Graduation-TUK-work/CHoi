#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class UUserWidget;

UCLASS()
class MYPROJECT_START_API ALobbyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ALobbyPlayerController();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    TSubclassOf<UUserWidget> LobbyWidgetClass;

    UPROPERTY()
    UUserWidget* LobbyWidget;
};
