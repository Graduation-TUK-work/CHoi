#include "InteractableActor.h"
#include "GameFramework/Character.h"

AInteractableActor::AInteractableActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AInteractableActor::BeginPlay()
{
    Super::BeginPlay();
}

void AInteractableActor::StartInteract_Implementation(ACharacter* Interactor)
{
    if (bIsRepaired) return;
    bIsBeingRepaired = true;
}

void AInteractableActor::UpdateInteract_Implementation(float DeltaTime)
{
    if (!bIsBeingRepaired || bIsRepaired) return;

    RepairProgress += DeltaTime / RepairDuration;
    RepairProgress = FMath::Clamp(RepairProgress, 0.f, 1.f);

    if (RepairProgress >= 1.f)
    {
        CompleteInteract_Implementation();
    }
}

void AInteractableActor::CancelInteract_Implementation()
{
    bIsBeingRepaired = false;
}

void AInteractableActor::CompleteInteract_Implementation()
{
    bIsBeingRepaired = false;
    bIsRepaired = true;
    UE_LOG(LogTemp, Warning, TEXT("Generator repaired!"));
}

float AInteractableActor::GetInteractDuration_Implementation() const
{
    return RepairDuration;
}