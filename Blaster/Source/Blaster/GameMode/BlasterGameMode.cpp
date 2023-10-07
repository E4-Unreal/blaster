﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"


void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	// TODO Score
	ABlasterPlayerState* AttackerPlayerState = AttackerController
	? Cast<ABlasterPlayerState>(AttackerController->PlayerState)
	: nullptr;

	ABlasterPlayerState* VictimPlayerState = VictimController
	? Cast<ABlasterPlayerState>(VictimController->PlayerState)
	: nullptr;

	if(AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}

	if(VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1.f);
	}

	if(EliminatedCharacter)
		EliminatedCharacter->ServerEliminate();
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if(EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
	}

	if(EliminatedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		const int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);
	}
}
