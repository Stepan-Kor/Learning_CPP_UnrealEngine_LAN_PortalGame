// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TeleportProjectHUD.generated.h"

UCLASS()
class ATeleportProjectHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATeleportProjectHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

