// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KillerGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_START_API AKillerGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;
	
};
