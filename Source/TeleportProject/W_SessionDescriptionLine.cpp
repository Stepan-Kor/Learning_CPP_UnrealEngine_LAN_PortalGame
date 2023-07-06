// Fill out your copyright notice in the Description page of Project Settings.


#include "W_SessionDescriptionLine.h"
#include "PortalsGameInstance.h"

bool UW_SessionDescriptionLine::Initialize()
{
	Super::Initialize();
	if (IsValid(ButtonJoin))ButtonJoin->OnClicked.AddDynamic(this, &UW_SessionDescriptionLine::JoinSession);
	else		UE_LOG(LogTemp, Warning, TEXT("Session Description widget: button not exist."));
	UE_LOG(LogTemp, Warning, TEXT("Session Description widget: initialized."));
	return true;
}

void UW_SessionDescriptionLine::SetVariables(int32 SessionIndex, FOnlineSessionSearchResult SearchResult)
{
	OnlineSearchResult = SearchResult;
	IndexOfSession = SessionIndex;
	SearchResult.Session.SessionSettings.Get(FName("SERVER_NAME_KEY"), SessionName );
	FString TempString = SessionName;
	TempString.Append("          ");
	MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
	bSessionIsLan = SearchResult.Session.SessionSettings.bIsLANMatch;
	CurrentPlayers = MaxPlayers - SearchResult.Session.NumOpenPublicConnections + bSessionIsLan;
	TempString.Append(FString::FromInt(CurrentPlayers));
	TempString.Append(" / ");
	TempString.Append(FString::FromInt(MaxPlayers));
	TempString.Append("       ");
	TempString.Append(bSessionIsLan ? "LAN" : "Online");	
	if(IsValid(MainText))MainText->SetText(FText::FromString(TempString));
}

void UW_SessionDescriptionLine::JoinSession()
{
	UE_LOG(LogTemp,Warning,TEXT("Session Description widget: Join button clicked."));
	UPortalsGameInstance* GameInstance = GetGameInstance<UPortalsGameInstance>();
	if (IsValid(GameInstance))GameInstance->JoinOnlineSession(FName(SessionName), OnlineSearchResult);
	else UE_LOG(LogTemp, Warning, TEXT("Session Description widget: cant get game instance."));
}
