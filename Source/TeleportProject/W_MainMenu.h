// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "W_SessionDescriptionLine.h"
#include "W_MainMenu.generated.h"

/**
 * 
 */
class UPortalsGameInstance;
//class UW_SessionDescriptionLine;
UCLASS()
class TELEPORTPROJECT_API UW_MainMenu : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget)) UButton* ButtonExit;
	UPROPERTY(meta = (BindWidget)) UButton* ButtonRefreshList;
	UPROPERTY(meta = (BindWidget)) UButton* ButtonCreateSession;
	UPROPERTY(meta = (BindWidget)) UScrollBox* ScrollBoxSessions;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* EditableTextBoxSessionName;
	UFUNCTION() void QuitGame();
	UFUNCTION() void FindSessions();
	UFUNCTION() void FindSessionsComplete(UPortalsGameInstance* PortalsGameInstance);
	UFUNCTION() void CreateSession();
	//UFUNCTION() void CreateSessionComplete();
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	UPortalsGameInstance* GameInstance;
	UPROPERTY(EditDefaultsOnly)TSubclassOf <UW_SessionDescriptionLine> SessionDescriptionLineClass = UW_SessionDescriptionLine::StaticClass();
	APlayerController* PlayerController;
};						
