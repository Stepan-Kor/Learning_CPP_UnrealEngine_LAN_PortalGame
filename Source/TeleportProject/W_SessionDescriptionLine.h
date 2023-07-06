// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "OnlineSessionSettings.h"
#include "W_SessionDescriptionLine.generated.h"

/**
 * 
 */
UCLASS()
class TELEPORTPROJECT_API UW_SessionDescriptionLine : public UUserWidget
{
	GENERATED_BODY()
protected:
	FOnlineSessionSearchResult OnlineSearchResult;
public:
	virtual bool Initialize() override;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MainText;
	UPROPERTY(meta = (BindWidget)) UButton* ButtonJoin;
	//UPROPERTY() UCheckBox* CheckBoxLAN;
	void SetVariables(int32 SessionIndex, FOnlineSessionSearchResult SearchResult);
	UFUNCTION()void JoinSession();
	FString SessionName = "Empty Session Name";
	int32 IndexOfSession;
	int8 MaxPlayers;
	int8 CurrentPlayers;
	bool bSessionIsLan;
};
