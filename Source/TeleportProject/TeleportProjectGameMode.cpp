// Copyright Epic Games, Inc. All Rights Reserved.

#include "TeleportProjectGameMode.h"
#include "TeleportProjectHUD.h"
#include "TeleportProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATeleportProjectGameMode::ATeleportProjectGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ATeleportProjectHUD::StaticClass();
}

void ATeleportProjectGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogTemp, Warning, TEXT("Game mode: New controller logged: %s"), *NewPlayer->GetName());
	AddControllerToPortalColorsList(NewPlayer);
}

FPortalColors ATeleportProjectGameMode::AddControllerToPortalColorsList(APlayerController* NewPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("Game mode: New controller added: %s"), *NewPlayer->GetName());
	if (PlayersPortalsColors.Contains(NewPlayer)) return *PlayersPortalsColors.Find(NewPlayer);
	FPortalColors NewColors;
	int32 AmountOfPlayers = PlayersPortalsColors.Num();
	if (AmountOfPlayers > PlayersPortalsColorsStandart.Num()-1) {
		NewColors.BluePortalColor = FColor::MakeRandomColor();
		NewColors.OrangePortalColor = FColor::MakeRandomColor();
	}
	else {
		NewColors = PlayersPortalsColorsStandart[AmountOfPlayers];
	}
	PlayersPortalsColors.Add(NewPlayer,NewColors);
	return NewColors;
}
