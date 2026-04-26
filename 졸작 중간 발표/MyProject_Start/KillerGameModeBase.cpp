// Fill out your copyright notice in the Description page of Project Settings.


#include "KillerGameModeBase.h"


void AKillerGameModeBase::StartPlay()
{
	Super::StartPlay();


	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("GAME MODE BASE"));
	}

}