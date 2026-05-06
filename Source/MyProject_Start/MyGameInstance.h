#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

UCLASS()
class MYPROJECT_START_API UMyGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Lobby")
    uint8 LocalSelectedRole = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    FName GameLevelName = TEXT("/Game/Stage_1");

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SelectKiller();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SelectRunner();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SelectSurvivor();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SelectKillerAndOpenGame();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SelectRunnerAndOpenGame();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SelectSurvivorAndOpenGame();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void OpenSelectedGameLevel();

    UFUNCTION(BlueprintPure, Category = "Lobby")
    FString GetSelectedGameModeOption() const;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SendReady();
};
