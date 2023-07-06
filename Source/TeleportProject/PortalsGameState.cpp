// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalsGameState.h"
#include "Kismet/GameplayStatics.h"
void APortalsGameState::BeginPlay()
{
	GetWorld()->GetFirstPlayerController()->SetShowMouseCursor(false);
	GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameOnly());
	if(HasAuthority())PortalColorOrange = FColor::MakeRandomColor();
	TArray <AActor*> TempActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(),TempActors);
	int8 i = 0;
	int8 count = 0;
	bool LocalFound=false;
	for(AActor* TempActor : TempActors){
		APlayerController* TempController = Cast<APlayerController>(TempActor);
		if (TempController)  { 
			count = TempController->NetPlayerIndex;
			LocalFound = true;
			UE_LOG(LogTemp, Warning, TEXT("Game State: player controller #%i."), count);
			if (!LocalFound) {
				LocalFound = true;
				count = i;
			}
		}
		i++;
	}
	if (LocalFound) switch (count) {

	case 1: 
		PortalColorOrange = FColor::Black;
		PortalColorBlue = FColor::Green; 
		break;
	case 2:
		PortalColorOrange = FColor::Magenta;
		PortalColorBlue = FColor::Silver;
		break;
	}
}
