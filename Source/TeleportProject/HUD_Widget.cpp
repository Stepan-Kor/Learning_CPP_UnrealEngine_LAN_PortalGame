// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD_Widget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"


void UHUD_Widget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ButtonExit)ButtonExit->OnClicked.AddDynamic(this,&UHUD_Widget::Exit);
	if (ButtonMainMenu)ButtonMainMenu->OnClicked.AddDynamic(this,&UHUD_Widget::ExitToMainMenu);
	if (ButtonContinue)ButtonContinue->OnClicked.AddDynamic(this,&UHUD_Widget::ContinuePlay);
}

APlayerController* UHUD_Widget::GetController()
{
	if (PlayerController) return PlayerController;
	PlayerController = GetWorld()->GetFirstPlayerController<APlayerController>();
	return PlayerController;
}

void UHUD_Widget::SwitchMenu(bool NewVisibility)
{
	if (!IsValid(VerticalBoxPanels))return;
	VerticalBoxPanels->SetVisibility(NewVisibility ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	if (!IsValid(GetController()))return;
	PlayerController->SetShowMouseCursor(NewVisibility);
	if(NewVisibility)PlayerController->SetInputMode(FInputModeUIOnly());
	else PlayerController->SetInputMode(FInputModeGameOnly());
}

void UHUD_Widget::Exit()
{
	UKismetSystemLibrary::QuitGame(GetWorld(),GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,true);
}

void UHUD_Widget::ExitToMainMenu()
{
	//GetWorld()->ServerTravel("/Game/FirstPersonCPP/Maps/FirstPersonExampleMap?Listen");
	UGameplayStatics::OpenLevel(GetWorld(),FName("/Game/MainMenu_Level?Listen"),true);
}

void UHUD_Widget::ContinuePlay()
{
	SwitchMenu(false);
}
