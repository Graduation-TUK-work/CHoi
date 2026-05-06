#include "EscapeDoor.h"

AEscapeDoor::AEscapeDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(Root);
}

void AEscapeDoor::OpenDoor_Implementation()
{
    if (bIsOpen) return;

    bIsOpen = true;
    UE_LOG(LogTemp, Warning, TEXT("The Escape Door is now OPEN!"));
}