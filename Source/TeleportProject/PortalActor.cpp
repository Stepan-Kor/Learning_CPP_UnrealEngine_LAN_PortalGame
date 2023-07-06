// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "TeleportProjectProjectile.h"
#include "TeleportProjectGameMode.h"
#include "PortalsGameState.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/GameState.h"
#include "Net/UnrealNetwork.h"
#include "TeleportProjectCharacter.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APortalActor::APortalActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh Component"));
	SetRootComponent(Mesh);
	Mesh->OnComponentBeginOverlap.AddDynamic(this, &APortalActor::OnOverlap);
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,  ECollisionResponse::ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel7,  ECollisionResponse::ECR_Overlap);
	//InitialLifeSpan = 15;
	bReplicates = true;
}


// Called when the game starts or when spawned
void APortalActor::BeginPlay()
{
	Super::BeginPlay();
	if(CurveOpenPortal){
		OpenPortalFloatProgress.BindUFunction(this,FName("TimeLineUpdate"));
		TimelineOpenPortal.AddInterpFloat(CurveOpenPortal, OpenPortalFloatProgress);
		TimelineOpenPortal.SetTimelineLength(0.5);
		TimelineOpenPortal.SetLooping(false);
		TimelineOpenPortal.PlayFromStart();
	}
	PortalsGameState = Cast <APortalsGameState>(GetWorld()->GetGameState());
	if (PortalsGameState)PortalsGameState->Portals.Add(this);
	SomeVirtualFunctionForTest();
	PortalsDelegateList.AddDynamic(this, &APortalActor::SomeTestForDelegateFunction);
	PortalsDelegateList.Broadcast(1);
}

void APortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APortalActor, bPortalDestroingSelf);
}

void APortalActor::Destroyed()
{
	Super::Destroyed();
	if (PortalsGameState)PortalsGameState->Portals.Remove(this);
}

void APortalActor::SomeVirtualFunctionForTest_Implementation()
{
	UE_LOG(LogTemp,Warning,TEXT("Portal Actor: Some unchanged Virtual Function called."));
}

void APortalActor::SomeTestForDelegateFunction(int SomeIntArgument)
{
	UE_LOG(LogTemp,Warning,TEXT("Portal Actor: delegate function trigered - int = %d"), SomeIntArgument);
}

void APortalActor::OnOverlap(UPrimitiveComponent* OverlapedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex, bool FromSweep, const FHitResult& HitResut)
{
	//UE_LOG(LogTemp,Warning,TEXT("Teleport Actor %s: Overlap Something: %s."),*this->GetName(), *OtherActor->GetName());
	if (!IsValid(AnotherPortal)) { 
		//UE_LOG(LogTemp, Warning, TEXT("Teleport Actor %s: donthave other portal."), *this->GetName());
		return;
	}

	if (CurrentlyIgnoringActor(OtherActor)) return;

	ATeleportProjectCharacter* TempPlayerChar = Cast <ATeleportProjectCharacter>(OtherActor);
	if (IsValid(TempPlayerChar)) { 

		//UE_LOG(LogTemp, Warning, TEXT("Teleport Actor: Overlap Something: %s"), *OtherActor->GetName());
		TempPlayerChar->TeleportPlayerCharacter(AnotherPortal,this);
		return;
	}
	ATeleportProjectProjectile* TempProj = Cast<ATeleportProjectProjectile>(OtherActor);
	if (IsValid(TempProj)) {
		FString TempString = bOrange ? "Blue" : "Orange";
		//UE_LOG(LogTemp, Warning, TEXT("Teleport Actor %s: speed before: %s."), *TempString, *TempProj->GetProjectileMovement()->Velocity.ToString());
		AnotherPortal->IgnoreThisActor(TempProj);
		FVector RelativeLocation = TempProj->GetActorLocation() - GetActorLocation();
		//FRotator TempRotator = UKismetMathLibrary::NormalizedDeltaRotator(this->GetActorForwardVector().Rotation(),(AnotherPortal->GetActorForwardVector() * -1).Rotation())*-1;
		FRotator TempRotator = AnotherPortal->GetActorRotation().RotateVector(-1 * GetActorForwardVector()).Rotation();
		TempProj->SetActorLocation(AnotherPortal->GetActorLocation() + TempRotator.RotateVector(RelativeLocation));
		TempProj->AddActorWorldRotation(TempRotator, false, nullptr, ETeleportType::None);
		TempProj->GetProjectileMovement()->Velocity = AnotherPortal->GetActorRotation().UnrotateVector( GetActorRotation().RotateVector(TempProj->GetProjectileMovement()->Velocity));

		//UE_LOG(LogTemp, Warning, TEXT("Teleport Actor %s: speed after: %s."), *TempString, *TempProj->GetProjectileMovement()->Velocity.ToString());
		//UE_LOG(LogTemp, Warning, TEXT("Teleport Actor %s: rotate projectile: %s."), *TempString, *TempRotator.ToString());
	}
}

bool APortalActor::CurrentlyIgnoringActor(AActor* Suspect)
{	
	if (IgnoreThisActors.Contains(Suspect)) {
		float TimeAdded = *IgnoreThisActors.Find(Suspect);
		if (GetWorld()->TimeSeconds - TimeAdded < 0.5) return true;
		else  IgnoreThisActors.Remove(Suspect); 
	}
	return false;
}

// Called every frame
void APortalActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimelineOpenPortal.TickTimeline(DeltaTime);

	if (bPortalDestroingSelf) {
		if (DestructionTimeLeft > 0) { 
			DestructionTimeLeft -= DeltaTime; 
			if (PortalMaterialInstance)PortalMaterialInstance->SetScalarParameterValue(FName("Opacity"),DestructionTimeLeft/ DestructionTimeMax);
		}
		else Destroy();
	}

}

void APortalActor::StartSetting(bool bIsOrange, APortalActor* SecondPortal, APlayerController* Controller)
{
	bOrange = bIsOrange;
	PortalsDelegateList.Broadcast(2);
	//FString TempString = bOrange ? "orange  " : "blue  ";
	//TempString.Append(GetActorLocation().ToString());
	//UE_LOG(LogTemp, Warning, TEXT("Teleport Actor: was created %s portal."), *TempString);
	USoundBase* TempSound = bIsOrange ? SpawnSoundOrange : SpawnSoundBlue;
	if (TempSound)UGameplayStatics::SpawnSoundAtLocation(GetWorld(), TempSound, GetActorLocation(),	GetActorRotation(), 1, 1, 0);
	if (SecondPortal) {
		AnotherPortal = SecondPortal;
		SecondPortal->AnotherPortal = this;
	}
	//else Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalMaterialInstance = UMaterialInstanceDynamic::Create(Mesh->GetMaterial(0),this);
	Mesh->SetMaterial(0,PortalMaterialInstance);
	FColor TempColor = PortalColor;/*
	APortalsGameState* TempGameState = Cast <APortalsGameState>(GetWorld()->GetGameState());
	if (TempGameState)TempColor = bOrange? TempGameState->PortalColorOrange : TempGameState->PortalColorBlue;
	else TempColor = bIsOrange ? FColor::Purple : FColor::Cyan;*/
	if (HasAuthority()) {
		ATeleportProjectGameMode* GameMode = Cast<ATeleportProjectGameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode && Controller) {
			UE_LOG(LogTemp, Warning, TEXT("Teleport Actor: controller sended %s."), *Controller->GetName());
			if (GameMode->PlayersPortalsColors.Contains(Controller))TempColor = bIsOrange ? GameMode->PlayersPortalsColors.Find(Controller)->OrangePortalColor :
				GameMode->PlayersPortalsColors.Find(Controller)->BluePortalColor;
			else {
				FPortalColors TempColors = GameMode->AddControllerToPortalColorsList(Controller);
				TempColor = bIsOrange ? TempColors.OrangePortalColor : TempColors.BluePortalColor;
				UE_LOG(LogTemp, Warning, TEXT("Teleport Actor: game mode dont content controller %s."), *Controller->GetName())
			}
		}
		else { UE_LOG(LogTemp, Warning, TEXT("Teleport Actor: cant get game mode %d or controller %d ."),IsValid(GameMode),IsValid(Controller)); }
	}
	if(TempColor==FColor(0,0,0))TempColor = bIsOrange ? FColor::Purple : FColor::Cyan;
	PortalColor = TempColor;
	PortalMaterialInstance->SetVectorParameterValue(FName("Color"), TempColor);
	PortalMaterialInstance->SetScalarParameterValue(FName("Time Constant"),bIsOrange ? 0 : 0.5);
	UParticleSystemComponent* TempPartComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SparksEmitter,
		FTransform(GetActorRotation(), GetActorLocation(), FVector(1,1,3)), true, EPSCPoolMethod::None, true);
	if (TempPartComp)TempPartComp->SetColorParameter(FName("Color"), TempColor);
	if (HasAuthority())Multi_ChangeColor(bIsOrange, SecondPortal,TempColor,Controller);
	//else UE_LOG(LogTemp, Warning, TEXT("Teleport Actor: particles was not created."));
}

void APortalActor::ClosePortal()
{
	bPortalDestroingSelf = true;
	AnotherPortal = nullptr;
	//TimelineOpenPortal.ReverseFromEnd();
	//FTimerHandle UnusedHandle;
	//GetWorldTimerManager().SetTimer(UnusedHandle, this, &APortalActor::TestDelayedFunction, 0.5f, false);
	//Destroy(); 
}

void APortalActor::IgnoreThisActor(AActor* IgnoreThis)
{
	
	IgnoreThisActors.Add(IgnoreThis,GetWorld()->TimeSeconds);
}

void APortalActor::TimeLineUpdate(float Value)
{
	SetActorScale3D(FVector (Value, Value, Value));
	//UE_LOG(LogTemp, Warning, TEXT("Teleport Actor %s: update own scale %s."), *this->GetName(), *FString::SanitizeFloat(Value));
}

void APortalActor::Multi_ChangeColor_Implementation(bool Orange, APortalActor* SecondPortal, FColor NewColor, APlayerController* Controller)
{
	if (!HasAuthority()) { 
		PortalColor = NewColor;
		StartSetting(Orange, nullptr, Controller); 
	}

}


