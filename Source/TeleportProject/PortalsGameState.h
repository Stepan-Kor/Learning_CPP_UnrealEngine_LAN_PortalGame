// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PortalsGameState.generated.h"

/**
 * 
 */
UCLASS()
class TELEPORTPROJECT_API APortalsGameState : public AGameStateBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
public:
	FColor PortalColorOrange = FColor::Orange;
	FColor PortalColorBlue=FColor::Turquoise;
	TArray <class APortalActor*> Portals;
	
};
