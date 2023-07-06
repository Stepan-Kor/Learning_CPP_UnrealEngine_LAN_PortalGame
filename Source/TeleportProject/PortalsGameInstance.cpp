// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalsGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
//#include "Kismet/GameplayStatics.h"

void UPortalsGameInstance::Init()
{
	Super::Init();	
	if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get()) { 
		SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid()) {
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this,&UPortalsGameInstance::FindSessionsComplete);
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UPortalsGameInstance::CreateSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this,&UPortalsGameInstance::JoinSessionComplete);
		}
	}
}

void UPortalsGameInstance::FindSessionsComplete(bool Succesfull)
{
	bSearchingForSessions = false;
	if (Succesfull)	OnSearchFinishedWarnList.Broadcast(this);
}

bool UPortalsGameInstance::FindSessions()
{
	if (bSearchingForSessions || !SessionInterface.IsValid())	return false;
	UE_LOG(LogTemp, Warning, TEXT("Game instance: Searching for sessions . . ."));
	bSearchingForSessions = true;
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	//SessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	SessionSearch->bIsLanQuery =true;
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE,true,EOnlineComparisonOp::Equals);
	SessionInterface->FindSessions(0,SessionSearch.ToSharedRef());
	return true;
}

bool UPortalsGameInstance::CreateSession(FName SessionName, FName HostName)
{
	if(bCreatingSession || !SessionInterface.IsValid())	return false;
	bCreatingSession = true;
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bIsDedicated = false;
	//SessionSettings.bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName()=="NULL" ? true : false;
	SessionSettings.bIsLANMatch = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.NumPublicConnections = 5;	
	const FName TempHostName = HostName == "" ?   "Empty Host Name": HostName;
	const FName TempSessionName = SessionName == "" ?  "Empty Session Name" : SessionName;
	//SessionSettings.Set(FName("SERVER_NAME_KEY"),TempName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	//SessionSettings.Set(FName("SERVER_NAME_KEY"), SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(FName("SERVER_NAME_KEY"), TempSessionName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(FName("SERVER_HOSTNAME_KEY"), TempHostName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionInterface->CreateSession(0, TempSessionName, SessionSettings);
	return true;
}

void UPortalsGameInstance::CreateSessionComplete(FName SessionName, bool Succesfull)
{
	bCreatingSession = false;
	if (!Succesfull) return;
	UE_LOG(LogTemp, Warning, TEXT("Game instance: Creation of session succeded %s"), *SessionName.ToString());
	GetWorld()->ServerTravel("/Game/FirstPersonCPP/Maps/FirstPersonExampleMap?Listen");
	//SessionInterface->
}

bool UPortalsGameInstance::JoinOnlineSession(FName SessionName, FOnlineSessionSearchResult& DesiredSession)
{
	if(!SessionInterface.IsValid() || SessionName == "" || bJoiningSession==true)return false;
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	int32 LocalUsers = 1;
	if (GameMode)LocalUsers = GameMode->GetNumPlayers();
	bJoiningSession = true;
	SessionInterface->JoinSession(LocalUsers,SessionName,DesiredSession);
	return true;
}
	
void UPortalsGameInstance::JoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type ResultType)
{
	bJoiningSession = false;
	if(ResultType==EOnJoinSessionCompleteResult::Success || ResultType== EOnJoinSessionCompleteResult::AlreadyInSession){
		APlayerController* Controller = GetWorld()->GetFirstPlayerController();
		if (!IsValid(Controller) || !SessionInterface.IsValid())		return;
		FString TravelAdress = "";
		SessionInterface->GetResolvedConnectString(SessionName, TravelAdress);
		if (TravelAdress == "")return;
		Controller->ClientTravel(TravelAdress,ETravelType::TRAVEL_Absolute);
	}
}
