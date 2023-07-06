// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "PortalsGameInstance.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionSearchFinishedDelegate, UPortalsGameInstance* , GameInstance);
UCLASS()
class TELEPORTPROJECT_API UPortalsGameInstance : public UGameInstance
{
	GENERATED_BODY()
protected:
	bool bSearchingForSessions = false;
	bool bCreatingSession = false;
	bool bJoiningSession = false;
public:
	IOnlineSessionPtr SessionInterface;
	void FindSessionsComplete(bool Succesfull);
	bool FindSessions();
	bool CreateSession(FName SessionName, FName HostName);
	void CreateSessionComplete(FName SessionName,bool Succesfull);
	bool JoinOnlineSession(FName SessionName,FOnlineSessionSearchResult& DesiredSession);
	void JoinSessionComplete(FName SessionName,EOnJoinSessionCompleteResult::Type ResultType);
	virtual void Init() override;
	FSessionSearchFinishedDelegate OnSearchFinishedWarnList;
	TSharedPtr <FOnlineSessionSearch> SessionSearch;
};
