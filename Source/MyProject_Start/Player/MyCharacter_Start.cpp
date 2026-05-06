// Fill out your copyright notice in the Description page of Project Settings.


#include "MyProject_Start/Player/MyCharacter_Start.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

// Sets default values
AMyCharacter_Start::AMyCharacter_Start()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->bUsePawnControlRotation = true;

}

// Called when the game starts or when spawned
void AMyCharacter_Start::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter_Start::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyCharacter_Start::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyCharacter_Start::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyCharacter_Start::MoveRight);

	PlayerInputComponent->BindAxis("TurnCamera", this, &AMyCharacter_Start::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMyCharacter_Start::LookUp);

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Vault", IE_Pressed, this, &AMyCharacter_Start::TryVault);
}


void AMyCharacter_Start::MoveForward(float InputValue)
{
	FVector ForwardDirection = GetActorForwardVector();
	AddMovementInput(ForwardDirection, InputValue);
}

void AMyCharacter_Start::MoveRight(float InputValue)
{
	FVector RightDirection = GetActorRightVector();
	AddMovementInput(RightDirection, InputValue);
}

void AMyCharacter_Start::Turn(float InputValue)
{
	AddControllerYawInput(InputValue);
}

void AMyCharacter_Start::LookUp(float InputValue)
{
	AddControllerPitchInput(InputValue);
}
	

void AMyCharacter_Start::TryVault()
{
	FVector Start = GetActorLocation() + FVector(0, 0, 50);
	FVector End = Start + GetActorForwardVector() * 100;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		FColor::Green,
		false,
		2.0f
	);

	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("장애물 감지됨!"));
	}
}

