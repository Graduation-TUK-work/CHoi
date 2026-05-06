#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyGameModeBase.generated.h"

class AController;
class APawn;

UCLASS()
class MYPROJECT_START_API AMyGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMyGameModeBase();

    virtual void BeginPlay() override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    void OnGeneratorRepaired();

protected:
    FVector GetSafeSpawnLocation() const;
    bool TryGetSpawnLocationOnPlane(const FString& PlaneName, FVector& OutLocation) const;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    TSubclassOf<APawn> SurvivorPawnClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    TSubclassOf<APawn> KillerPawnClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FVector SafeSpawnLocation = FVector(0.0f, 0.0f, 800.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FRotator SafeSpawnRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameLogic")
    int32 TotalGenerators = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameLogic")
    int32 RepairedGenerators = 0;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameLogic")
    void OpenEscapeDoor();
};
