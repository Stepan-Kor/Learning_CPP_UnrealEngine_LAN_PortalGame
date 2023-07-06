// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TeleportProjectGameMode.generated.h"
USTRUCT(BlueprintType) struct FPortalColors {
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite) FColor OrangePortalColor;
	UPROPERTY(BlueprintReadWrite) FColor BluePortalColor;
};
UCLASS(minimalapi)
class ATeleportProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATeleportProjectGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	FPortalColors AddControllerToPortalColorsList(APlayerController* NewPlayer);

	TMap <APlayerController*, FPortalColors> PlayersPortalsColors;
	UPROPERTY(BlueprintReadWrite,EditAnywhere)TArray <FPortalColors> PlayersPortalsColorsStandart = {
		FPortalColors{FColor::Orange,FColor::Turquoise},
		FPortalColors{FColor::Red,FColor::Blue},
		FPortalColors{FColor::Yellow,FColor::Emerald}
	};
};



