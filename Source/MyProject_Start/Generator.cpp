#include "Generator.h"
#include "Algo/Sort.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject_Start/MyGameModeBase.h"

AGenerator::AGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(true);
    bIsRepaired = false;
    bIsBeingRepaired = false;
    RepairProgress = 0.0f;
    RepairDuration = 5.0f;
    CurrentInteractor = nullptr;
}

void AGenerator::BeginPlay()
{
    Super::BeginPlay();
    AssignGeneratedIdIfNeeded();

    UE_LOG(LogTemp, Warning, TEXT("Generator Spawned and Ready. ID: %d"), GeneratorId);
}

int32 AGenerator::GetGeneratorId() const
{
    return GeneratorId;
}

float AGenerator::GetRepairProgress() const
{
    return RepairProgress;
}

bool AGenerator::IsBeingRepaired() const
{
    return bIsBeingRepaired;
}

void AGenerator::StartInteract_Implementation(ACharacter* Interactor)
{
    if (bIsRepaired) return;

    SetRepairState(true, false, RepairProgress);
    CurrentInteractor = Interactor;
}

void AGenerator::UpdateInteract_Implementation(float DeltaTime)
{
    if (!bIsBeingRepaired || bIsRepaired) return;

    const float NewRepairProgress = FMath::Clamp(RepairProgress + (DeltaTime / RepairDuration), 0.f, 1.f);
    SetRepairState(true, false, NewRepairProgress);

    if (RepairProgress >= 1.f)
    {
        CompleteInteract_Implementation();
        return;
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(1, DeltaTime, FColor::Cyan,
            FString::Printf(TEXT("Repair Progress: %.2f %%"), RepairProgress * 100.f));
    }
}

void AGenerator::CompleteInteract_Implementation()
{
    if (bIsRepaired) return;

    SetRepairState(false, true, 1.f);

    UE_LOG(LogTemp, Warning, TEXT("Generator repaired! ID: %d"), GeneratorId);

    if (CurrentInteractor)
    {
        CurrentInteractor = nullptr;
    }

    AMyGameModeBase* GM = Cast<AMyGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM)
    {
        GM->OnGeneratorRepaired();
    }
}

void AGenerator::CancelInteract_Implementation()
{
    if (bIsRepaired) return;

    SetRepairState(false, false, RepairProgress);
    CurrentInteractor = nullptr;
    UE_LOG(LogTemp, Warning, TEXT("Generator repair cancelled. ID: %d Progress: %.2f"), GeneratorId, RepairProgress);
}

float AGenerator::GetInteractDuration_Implementation() const
{
    return RepairDuration;
}

void AGenerator::ApplyNetworkRepairState(bool bInBeingRepaired, bool bInRepaired, float InRepairProgress)
{
    SetRepairState(bInBeingRepaired, bInRepaired, InRepairProgress);

    if (!bInBeingRepaired)
    {
        CurrentInteractor = nullptr;
    }
}

void AGenerator::AssignGeneratedIdIfNeeded()
{
    if (GeneratorId >= 0 || !GetWorld())
    {
        return;
    }

    TArray<AActor*> FoundGenerators;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGenerator::StaticClass(), FoundGenerators);

    Algo::Sort(FoundGenerators, [](const AActor* Left, const AActor* Right)
        {
            return Left && Right && Left->GetName() < Right->GetName();
        });

    GeneratorId = FoundGenerators.IndexOfByKey(this);
}

void AGenerator::SetRepairState(bool bInBeingRepaired, bool bInRepaired, float InRepairProgress)
{
    const bool bWasBeingRepaired = bIsBeingRepaired;
    const bool bWasRepaired = bIsRepaired;
    const float OldRepairProgress = RepairProgress;

    bIsBeingRepaired = bInBeingRepaired;
    bIsRepaired = bInRepaired;
    RepairProgress = FMath::Clamp(InRepairProgress, 0.f, 1.f);

    if (bWasBeingRepaired != bIsBeingRepaired || bWasRepaired != bIsRepaired || !FMath::IsNearlyEqual(OldRepairProgress, RepairProgress))
    {
        OnRepairStateChanged(bIsBeingRepaired, bIsRepaired, RepairProgress);
    }

    if (!bWasRepaired && bIsRepaired)
    {
        OnRepairCompleted();
    }
}