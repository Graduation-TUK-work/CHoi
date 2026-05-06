#include "KillerGameModeBase.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AKillerGameModeBase::AKillerGameModeBase()
{
    static ConstructorHelpers::FClassFinder<APawn> KillerPawnClass(TEXT("/Game/BP/BP_KillerCharacter"));
    if (KillerPawnClass.Succeeded())
    {
        DefaultPawnClass = KillerPawnClass.Class;
    }
}

void AKillerGameModeBase::StartPlay()
{
    Super::StartPlay();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Killer GameMode"));
    }
}

void AKillerGameModeBase::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer)
    {
        return;
    }

    const FTransform SpawnTransform(SafeSpawnRotation, SafeSpawnLocation);
    APawn* NewPawn = SpawnDefaultPawnAtTransform(NewPlayer, SpawnTransform);
    if (NewPawn)
    {
        NewPlayer->SetControlRotation(SafeSpawnRotation);
        NewPlayer->Possess(NewPawn);
        UE_LOG(LogTemp, Warning, TEXT("Spawned killer at safe location: %s"), *SafeSpawnLocation.ToString());
        return;
    }

    Super::RestartPlayer(NewPlayer);
}
