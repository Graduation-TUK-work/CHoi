#include "Generator.h"
#include "GameFramework/Character.h"

AGenerator::AGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGenerator::BeginPlay()
{
    Super::BeginPlay();
}

void AGenerator::StartInteract_Implementation(ACharacter* Interactor)
{
    if (bIsRepaired) return;

    bIsBeingRepaired = true;
}

void AGenerator::UpdateInteract_Implementation(float DeltaTime)
{
    if (!bIsBeingRepaired || bIsRepaired) return;

    RepairProgress += DeltaTime / RepairDuration;
    RepairProgress = FMath::Clamp(RepairProgress, 0.f, 1.f);

    if (RepairProgress >= 1.f)
    {
        CompleteInteract_Implementation();
    }
    // 화면 왼쪽 상단에 실시간으로 진행도 출력 (0.0 ~ 1.0)
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(1, DeltaTime, FColor::Cyan,
            FString::Printf(TEXT("Repair Progress: %.2f %%"), RepairProgress * 100.f));
    }
}

void AGenerator::CancelInteract_Implementation()
{
    // 진행도 유지, 수리 중단만
    bIsBeingRepaired = false;
}

void AGenerator::CompleteInteract_Implementation()
{
    bIsBeingRepaired = false;
    bIsRepaired = true;

    UE_LOG(LogTemp, Warning, TEXT("Generator repaired!"));
}

float AGenerator::GetInteractDuration_Implementation() const
{
    return RepairDuration;
}