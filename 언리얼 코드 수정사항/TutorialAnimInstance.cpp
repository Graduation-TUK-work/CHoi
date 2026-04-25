// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialAnimInstance.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "MyProject_Start/Player/TutorialCharacter.h"


void UTutorialAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	auto Pawn = TryGetPawnOwner();

	if (IsValid(Pawn))
	{
		auto Character = Cast<ATutorialCharacter>(Pawn);
		if (Character)
		{
			if (Character->IsPlayerControlled())
			{
				Vertical = Character->MoveForwardValue;
				Horizontal = Character->MoveRightValue;
			}
			else
			{
				Vertical = Character->RemoteForwardValue;
				Horizontal = Character->RemoteRightValue;
			}
		}
	}
}
