#include "MyGameModeBase.h"

#include "EscapeDoor.h"
#include "Generator.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameInstance.h"
#include "Shared.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"

namespace
{
    FString NormalizePlaneName(const FString& Name)
    {
        FString Normalized = Name.ToLower();
        Normalized.ReplaceInline(TEXT(" "), TEXT(""));
        Normalized.ReplaceInline(TEXT("_"), TEXT(""));
        Normalized.ReplaceInline(TEXT("-"), TEXT(""));
        return Normalized;
    }
}

AMyGameModeBase::AMyGameModeBase()
{
    static ConstructorHelpers::FClassFinder<APawn> SurvivorPawnFinder(TEXT("/Game/BP/BP_TutorialCharacter"));
    if (SurvivorPawnFinder.Succeeded())
    {
        SurvivorPawnClass = SurvivorPawnFinder.Class;
        DefaultPawnClass = SurvivorPawnFinder.Class;
    }

    static ConstructorHelpers::FClassFinder<APawn> KillerPawnFinder(TEXT("/Game/BP/BP_KillerCharacter"));
    if (KillerPawnFinder.Succeeded())
    {
        KillerPawnClass = KillerPawnFinder.Class;
    }

    TotalGenerators = 0;
    RepairedGenerators = 0;
}

void AMyGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> FoundGenerators;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGenerator::StaticClass(), FoundGenerators);

    TotalGenerators = FoundGenerators.Num();
    RepairedGenerators = 0;

    UE_LOG(LogTemp, Warning, TEXT("Map Loaded. Total Generators: %d"), TotalGenerators);
}

UClass* AMyGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    if (UMyGameInstance* GI = GetGameInstance<UMyGameInstance>())
    {
        if (GI->LocalSelectedRole == ROLE_KILLER && KillerPawnClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Spawning selected KILLER pawn."));
            return KillerPawnClass;
        }
    }

    if (SurvivorPawnClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Spawning selected SURVIVOR pawn."));
        return SurvivorPawnClass;
    }

    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AMyGameModeBase::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer)
    {
        return;
    }

    const FVector SpawnLocation = GetSafeSpawnLocation();
    const FTransform SpawnTransform(SafeSpawnRotation, SpawnLocation);
    APawn* NewPawn = SpawnDefaultPawnAtTransform(NewPlayer, SpawnTransform);
    if (NewPawn)
    {
        NewPlayer->SetControlRotation(SafeSpawnRotation);
        NewPlayer->Possess(NewPawn);
        UE_LOG(LogTemp, Warning, TEXT("Spawned player at safe location: %s"), *SpawnLocation.ToString());
        return;
    }

    Super::RestartPlayer(NewPlayer);
}

FVector AMyGameModeBase::GetSafeSpawnLocation() const
{
    if (UMyGameInstance* GI = GetGameInstance<UMyGameInstance>())
    {
        FVector NamedPlaneLocation;
        const FString TargetPlaneName = (GI->LocalSelectedRole == ROLE_KILLER) ? TEXT("Plane 11") : TEXT("Plane 16");
        if (TryGetSpawnLocationOnPlane(TargetPlaneName, NamedPlaneLocation))
        {
            UE_LOG(LogTemp, Warning, TEXT("Using %s spawn location: %s"), *TargetPlaneName, *NamedPlaneLocation.ToString());
            return NamedPlaneLocation;
        }

        UE_LOG(LogTemp, Warning, TEXT("Could not find %s. Falling back to largest plane."), *TargetPlaneName);
    }

    FVector BestLocation = SafeSpawnLocation;
    float BestArea = 0.0f;

    UWorld* World = GetWorld();
    if (!World)
    {
        return BestLocation;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor->IsHidden())
        {
            continue;
        }

        TArray<UPrimitiveComponent*> PrimitiveComponents;
        Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
        for (UPrimitiveComponent* Primitive : PrimitiveComponents)
        {
            if (!Primitive || !Primitive->IsCollisionEnabled())
            {
                continue;
            }

            const FBox Bounds = Primitive->Bounds.GetBox();
            const FVector Size = Bounds.GetSize();
            const float Area = Size.X * Size.Y;
            if (Area > BestArea && Size.X > 300.0f && Size.Y > 300.0f && Size.Z < 300.0f)
            {
                BestArea = Area;
                BestLocation = FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z + 160.0f);
            }
        }
    }

    return BestLocation;
}

bool AMyGameModeBase::TryGetSpawnLocationOnPlane(const FString& PlaneName, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const FString TargetName = NormalizePlaneName(PlaneName);

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor->IsHidden())
        {
            continue;
        }

        const FString ActorName = NormalizePlaneName(Actor->GetActorNameOrLabel());
        if (!ActorName.Contains(TargetName))
        {
            continue;
        }

        TArray<UPrimitiveComponent*> PrimitiveComponents;
        Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
        for (UPrimitiveComponent* Primitive : PrimitiveComponents)
        {
            if (!Primitive || !Primitive->IsCollisionEnabled())
            {
                continue;
            }

            const FBox Bounds = Primitive->Bounds.GetBox();
            const FVector Size = Bounds.GetSize();
            if (Size.X > 50.0f && Size.Y > 50.0f)
            {
                OutLocation = FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z + 160.0f);
                return true;
            }
        }

        const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
        if (ActorBounds.IsValid)
        {
            OutLocation = FVector(ActorBounds.GetCenter().X, ActorBounds.GetCenter().Y, ActorBounds.Max.Z + 160.0f);
            return true;
        }
    }

    return false;
}

void AMyGameModeBase::OnGeneratorRepaired()
{
    RepairedGenerators++;
    UE_LOG(LogTemp, Warning, TEXT("Generator Repaired! Progress: %d / %d"), RepairedGenerators, TotalGenerators);

    if (RepairedGenerators == TotalGenerators)
    {
        OpenEscapeDoor();
    }
}

void AMyGameModeBase::OpenEscapeDoor_Implementation()
{
    TArray<AActor*> FoundDoors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEscapeDoor::StaticClass(), FoundDoors);

    UE_LOG(LogTemp, Warning, TEXT("GameMode: Found %d Escape Doors in map."), FoundDoors.Num());

    for (AActor* DoorActor : FoundDoors)
    {
        AEscapeDoor* Door = Cast<AEscapeDoor>(DoorActor);
        if (Door)
        {
            UE_LOG(LogTemp, Warning, TEXT("GameMode: Calling OpenDoor on %s"), *Door->GetName());
            Door->OpenDoor();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("GameMode: Found actor but failed to cast to AEscapeDoor!"));
        }
    }
}
