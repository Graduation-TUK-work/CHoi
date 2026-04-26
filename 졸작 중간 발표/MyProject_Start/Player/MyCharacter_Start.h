// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyCharacter_Start.generated.h"

UCLASS()
class MYPROJECT_START_API AMyCharacter_Start : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter_Start();
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(EditAnywhere)
	class UCameraComponent* Camera;

	void MoveForward(float InputValue);
	void MoveRight(float InputValue);
	void Turn(float InputValue);
	void LookUp(float InputValue);
	void TryVault();
	bool bIsVaulting = false;
};

