// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "HUD_Widget.generated.h"

/**
 * 
 */
UCLASS()
class TELEPORTPROJECT_API UHUD_Widget : public UUserWidget
{
	GENERATED_BODY()
		UPROPERTY(meta = (BindWidget)) UButton* ButtonExit;
		UPROPERTY(meta = (BindWidget)) UButton* ButtonMainMenu;
		UPROPERTY(meta = (BindWidget)) UButton* ButtonContinue;
		UPROPERTY(meta = (BindWidget)) UVerticalBox* VerticalBoxPanels;
		UFUNCTION() void Exit();
		UFUNCTION() void ExitToMainMenu();
		UFUNCTION() void ContinuePlay();
protected:
	virtual void NativeConstruct() override;
	APlayerController* GetController();
	APlayerController* PlayerController;
public:
	void SwitchMenu(bool NewVisibility);
	
};
