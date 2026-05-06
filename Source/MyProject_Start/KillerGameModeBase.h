#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KillerGameModeBase.generated.h"

class AController;

UCLASS()
class MYPROJECT_START_API AKillerGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AKillerGameModeBase();

    virtual void StartPlay() override;
    virtual void RestartPlayer(AController* NewPlayer) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FVector SafeSpawnLocation = FVector(0.0f, 0.0f, 800.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FRotator SafeSpawnRotation = FRotator::ZeroRotator;
};
