// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TutorialAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_START_API UTutorialAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pawn, Meta = (AllowPrivateAccess = true))
	float Horizontal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pawn, Meta = (AllowPrivateAccess = true))
	float Vertical;
	
};
