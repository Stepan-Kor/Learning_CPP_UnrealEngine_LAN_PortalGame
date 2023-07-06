// Fill out your copyright notice in the Description page of Project Settings.


#include "W_MainMenu.h"
#include "PortalsGameInstance.h"
#include "Kismet/GameplayStatics.h"

bool UW_MainMenu::Initialize()
{
	Super::Initialize();
	if (ButtonExit)ButtonExit->OnClicked.AddDynamic(this, &UW_MainMenu::QuitGame);
	if (ButtonRefreshList)ButtonRefreshList->OnClicked.AddDynamic(this, &UW_MainMenu::FindSessions);
	if (ButtonCreateSession)ButtonCreateSession->OnClicked.AddDynamic(this, &UW_MainMenu::CreateSession);
	GameInstance = Cast<UPortalsGameInstance>(GetGameInstance());
	if (GameInstance)	 GameInstance->OnSearchFinishedWarnList.AddDynamic(this, &UW_MainMenu::FindSessionsComplete);
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController) { PlayerController->SetInputMode(FInputModeUIOnly()); PlayerController->SetShowMouseCursor(true); }
	/*
	TArray <AActor*> TempActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),APlayerController::StaticClass(),TempActors);
	UE_LOG(LogTemp,Warning,TEXT("Widget Main menu: Amount of player controllers %i"),TempActors.Num());*/
	return true;
}

void UW_MainMenu::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController,EQuitPreference::Quit,true);
}

void UW_MainMenu::FindSessions()
{
	if (!IsValid(GameInstance))	return;
	bool SearchStarted = GameInstance->FindSessions();
	UE_LOG(LogTemp, Warning, TEXT("Main Menu Widget: sessions search start %s."), SearchStarted);

	if (!SearchStarted)		return;
	ButtonRefreshList->SetIsEnabled(false);
	ScrollBoxSessions->ClearChildren();
}

void UW_MainMenu::FindSessionsComplete(UPortalsGameInstance* PortalsGameInstance)
{
	if (!IsValid(GameInstance)) return;
	UW_SessionDescriptionLine* TempWidget;
	if(ButtonRefreshList)ButtonRefreshList->SetIsEnabled(true);
	UE_LOG(LogTemp,Warning,TEXT("Main Menu Widget: sessions search complete - %i results."), GameInstance->SessionSearch->SearchResults.Num());
	int8 Counter = 0;
	for(FOnlineSessionSearchResult Result : GameInstance->SessionSearch->SearchResults){
		if (Counter > 1000) break;
		if (Result.IsValid()) {
			FString TempString = "Session Description ";
			TempString.Append(FString::FromInt(Counter));
			TempWidget = CreateWidget<UW_SessionDescriptionLine>(this, SessionDescriptionLineClass, FName(TempString));
			if (IsValid(ScrollBoxSessions) && IsValid(TempWidget)) {
				TempWidget->SetVariables(Counter, Result);
				ScrollBoxSessions->AddChild(TempWidget);
				UE_LOG(LogTemp, Warning, TEXT("Main Menu Widget: session found %i - %s."), Counter, *TempString);
			}
			else UE_LOG(LogTemp, Warning, TEXT("Main Menu Widget: cant create description of session."));
		}
		Counter++;
	}
	
}

void UW_MainMenu::CreateSession()
{
	if (!GameInstance || !EditableTextBoxSessionName) return;
	FName TempSessionName;
	TempSessionName = FName ( EditableTextBoxSessionName->GetText().ToString());
	if(TempSessionName == "") TempSessionName = "Empty Session Name";
	FName HostName = FName("Host Name");
	GameInstance->CreateSession(TempSessionName, HostName);
}

void UW_MainMenu::NativeConstruct()
{
	Super::NativeConstruct();
	//SessionDescriptionLineClass = UW_SessionDescriptionLine::StaticClass();
}
