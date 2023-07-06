// Copyright Epic Games, Inc. All Rights Reserved.

#include "TeleportProjectCharacter.h"
#include "HUD_Widget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "PortalsGameState.h"
//#include "Kismet/KismetSystemLibrary.h"
#include "TeleportProjectProjectile.h"
#include "Animation/AnimInstance.h"
#include "PortalActor.h"
//#include "CoreNet.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include <memory>

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ATeleportProjectCharacter

ATeleportProjectCharacter::ATeleportProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	SetActorTickEnabled ( true);
	GetCharacterMovement()->AirControl = 0.75;
	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	bReplicates = true;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(Mesh1P,TEXT("GripPoint"));

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.


	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void ATeleportProjectCharacter::AnotherTestDelegateFunction(int IntegerArgument) 
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter Actor: delegate function trigered - int = %d"), IntegerArgument);
}

void ATeleportProjectCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	HandsAnimationsInstance = Mesh1P->GetAnimInstance();
	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
	GameState = Cast<APortalsGameState>(GetWorld()->GetGameState());
	PlayerController = Cast<APlayerController>(GetController());
	if (HUDWidgetClass) {
		HUDWidget = Cast<UHUD_Widget>(CreateWidget(GetWorld(), HUDWidgetClass, FName("HUD Widget")));
		if (HUDWidget)HUDWidget->AddToPlayerScreen();
	}
	
	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
	APortalsGameState* TempState = Cast<APortalsGameState>(GetWorld()->GetGameState());
	if (TempState && PlayerController) {

		UE_LOG(LogTemp, Warning, TEXT("Player Character %s: controller have ID %i."), *this->GetName(), PlayerController->GetUniqueID());
		switch (PlayerController->GetUniqueID()) {
			case 0: 
				break;
			case 1:
				TempState->PortalColorBlue = FColor::Blue;
				TempState->PortalColorOrange = FColor::Emerald;
				break;
			case 2:
				TempState->PortalColorBlue = FColor::Green;
				TempState->PortalColorOrange = FColor::Magenta;
				break;
		}
	}
	//else		UE_LOG(LogTemp, Warning, TEXT("Player Character %s: controller have no GameState or Controller."), *this->GetName());
}

void ATeleportProjectCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION (ATeleportProjectCharacter, CameraPitch, COND_AutonomousOnly);
	//DOREPLIFETIME_CONDITION (ATeleportProjectCharacter, CameraPitch, COND_SkipOwner);
	//DOREPLIFETIME(ATeleportProjectCharacter, CameraPitch);
}

void ATeleportProjectCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CameraUpdateReady = true;

}

//////////////////////////////////////////////////////////////////////////
// Input

void ATeleportProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Shot Orange Portal",IE_Pressed,this,&ATeleportProjectCharacter::ShotOrangePortal);
	PlayerInputComponent->BindAction("Shot Blue Portal",IE_Pressed,this,&ATeleportProjectCharacter::ShotBluePortal);
	PlayerInputComponent->BindAction("Pause",IE_Pressed,this,&ATeleportProjectCharacter::OpenMenu);
	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &ATeleportProjectCharacter::RunStart);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &ATeleportProjectCharacter::RunEnd);
	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATeleportProjectCharacter::OnFire);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ATeleportProjectCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ATeleportProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATeleportProjectCharacter::MoveRight);
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ATeleportProjectCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ATeleportProjectCharacter::LookUpCustom);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ATeleportProjectCharacter::LookUpAtRate);
}

void ATeleportProjectCharacter::OnFire() { 
	UGameplayStatics::GetGameState(GetWorld());
	FString TempString = IsValid(GetWorld()->GetGameState()) ? "GameState is valid" : "GameState is not valid";
	TempString.Append(IsValid(GetGameInstance()) ? "   GameInstance is valid" : "   GameInstance is not valid");
	TempString.Append(GetPlayerState() ? "   PlayerState is valid" : "   PlayerState is not valid");
	TempString.Append(GetWorld()->GetAuthGameMode() ? "   GameMode is valid" : "   GameMode is not valid");
	UE_LOG(LogTemp, Warning, TEXT("Player Character: %s."), *TempString);
	
	
	
	Server_BallShot();  }

void ATeleportProjectCharacter::BallShot(bool Blank)
{
	// try and fire a projectile
	if (ProjectileClass != nullptr && !Blank)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			if (bUsingMotionControllers)
			{
				const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
				const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
				World->SpawnActor<ATeleportProjectProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			}
			else
			{
				const FRotator SpawnRotation = FirstPersonCameraComponent->GetComponentRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<ATeleportProjectProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void ATeleportProjectCharacter::Server_BallShot_Implementation() { BallShot(); Multi_BallShot(); }

void ATeleportProjectCharacter::Multi_BallShot_Implementation(){	BallShot(true);}

void ATeleportProjectCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ATeleportProjectCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void ATeleportProjectCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void ATeleportProjectCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void ATeleportProjectCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ATeleportProjectCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ATeleportProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	if(Rate!=0)AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ATeleportProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	if (Rate != 0) { 
		AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds()); 

		if (CameraUpdateReady) {
			CameraPitch = FirstPersonCameraComponent->GetRelativeRotation().Pitch;
			Server_UpdateCameraPitch(CameraPitch);
			CameraUpdateReady = false;
		}
	}
}

void ATeleportProjectCharacter::LookUpCustom(float Value)
{
	if (Value == 0)return;
	AddControllerPitchInput(Value);
	if (CameraUpdateReady) {
		CameraPitch = FirstPersonCameraComponent->GetRelativeRotation().Pitch;
		Server_UpdateCameraPitch(CameraPitch);
		CameraUpdateReady = false;
		}
}

bool ATeleportProjectCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ATeleportProjectCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &ATeleportProjectCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &ATeleportProjectCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}

void ATeleportProjectCharacter::ShotOrangePortal()
{
	bool bWasCreated = SpawnPortal(true);
	//FString TempString = bWasCreated ? "was created" : "was not created";
	//UE_LOG(LogTemp, Warning, TEXT("Player Character: After orange shot, portal %s ."), *TempString);

}

void ATeleportProjectCharacter::ShotBluePortal()
{
	bool bWasCreated = SpawnPortal(false);
	//FString TempString = bWasCreated ? "was created" : "was not created";
	//UE_LOG(LogTemp, Warning, TEXT("Player Character: After blue shot, portal %s ."), *TempString);
}

void ATeleportProjectCharacter::RunStart()
{
	GetCharacterMovement()->MaxWalkSpeed = 1100;
	GetCharacterMovement()->MaxAcceleration = 3300;
	Server_ChangeMovementSpeed(1100);
}

void ATeleportProjectCharacter::RunEnd()
{
	GetCharacterMovement()->MaxWalkSpeed = 600;
	GetCharacterMovement()->MaxAcceleration = 1800;
	if (HasAuthority())Multi_ChangeMovementSpeed(600);
	else Server_ChangeMovementSpeed(600);

}

void ATeleportProjectCharacter::TeleportPlayerCharacter(APortalActor* TargetPortal, APortalActor* EnterPortal)
{
	if (!IsValid(TargetPortal) || !IsValid(EnterPortal)) return;
	TargetPortal->IgnoreThisActor(this);
	FHitResult TempHit;
	SetActorLocation(TargetPortal->GetActorLocation()+TargetPortal->GetActorForwardVector()*( GetCapsuleComponent()->GetScaledCapsuleRadius()+10),
		false,nullptr,ETeleportType::None);

	
	FRotator TempNewRotator; 
	float NewDeltaYaw=0;
	if (TargetPortal->GetActorForwardVector().Z < 0.5 && EnterPortal->GetActorForwardVector().Z < 0.5) {
		
			TempNewRotator = UKismetMathLibrary::NormalizedDeltaRotator(FRotator(0, TargetPortal->GetActorRotation().Yaw, 0),
				FRotator(0, (EnterPortal->GetActorForwardVector() * -1).Rotation().Yaw, 0));
			NewDeltaYaw = TempNewRotator.Yaw;
		if (PlayerController) {
			Controller->SetControlRotation(FRotator(Controller->GetControlRotation().Pitch, NewDeltaYaw + GetActorRotation().Yaw, 0));
		}
		
		//UE_LOG(LogTemp, Warning, TEXT("Player Character: Added rotation %s."), *FString::SanitizeFloat(NewDeltaYaw));
	}

	UE_LOG(LogTemp, Warning, TEXT("Player Character: velocity before %s."), *GetCharacterMovement()->Velocity.ToString());

	if (HasAuthority()) {
		Multi_TurnController(FRotator(Controller->GetControlRotation().Pitch, NewDeltaYaw + GetActorRotation().Yaw, 0),
			TargetPortal->GetActorRotation().RotateVector(EnterPortal->GetActorRotation().UnrotateVector(GetCharacterMovement()->Velocity * -1))			);
	}
	GetCharacterMovement()->Velocity = TargetPortal->GetActorRotation().RotateVector(EnterPortal->GetActorRotation().UnrotateVector(GetCharacterMovement()->Velocity *-1));
	//UE_LOG(LogTemp, Warning, TEXT("Player Character: velocity after %s."), *GetCharacterMovement()->Velocity.ToString());
	//GetCharacterMovement()->Velocity = EnterPortal->GetActorRotation().UnrotateVector(TargetPortal->GetActorRotation().RotateVector(GetCharacterMovement()->Velocity));
}

void ATeleportProjectCharacter::Multi_TurnController_Implementation(FRotator NewRotation, FVector NewVelocity)
{
	if (PlayerController && !HasAuthority()) {
		PlayerController->SetControlRotation(NewRotation);
	GetCharacterMovement()->Velocity = NewVelocity;
	UE_LOG(LogTemp, Warning, TEXT("Player Character: multi turn character- new velocity %s."), *GetCharacterMovement()->Velocity.ToString());
	}
}

void ATeleportProjectCharacter::OpenMenu()
{
	if (!IsValid(HUDWidget))return;
	HUDWidget->SwitchMenu(true);
}

void ATeleportProjectCharacter::Server_ChangeMovementSpeed_Implementation(float Value)
{
	Multi_ChangeMovementSpeed(Value);
}

void ATeleportProjectCharacter::Multi_ChangeMovementSpeed_Implementation(float Value)
{
	GetCharacterMovement()->MaxWalkSpeed = Value;
	GetCharacterMovement()->MaxAcceleration = Value*3;
}

void ATeleportProjectCharacter::UpdateCamera()
{	
	//if (!bShouldUpdateCamera)return;
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(CameraPitch, 0, 0));
	//UE_LOG(LogTemp, Warning, TEXT("Player Character %s: Camera Updated."), *GetName());
}

void ATeleportProjectCharacter::Server_UpdateCameraPitch_Implementation(float Value){	
	CameraPitch = Value;
	UpdateCamera();
	//UE_LOG(LogTemp, Warning, TEXT("Player Character: camera pitch updated %s."), *FString::SanitizeFloat(CameraPitch));
}

bool ATeleportProjectCharacter::SpawnPortal(bool bOrange)
{
	FHitResult HitResultTemp;
	//FVector VectorStart = FP_Gun->GetSocketLocation(FName ("Muzzle"));
	FVector VectorStart = FirstPersonCameraComponent->GetComponentLocation();
	//FVector VectorEnd = VectorStart +FP_Gun->GetRightVector()*2500;
	FVector VectorEnd = VectorStart + FirstPersonCameraComponent->GetForwardVector() * 10000;
	FCollisionQueryParams QueryParams;
#if WITH_EDITOR
	QueryParams.bDebugQuery = true;
#endif
	QueryParams.AddIgnoredActor(this);
	//QueryParams.
	FCollisionResponseParams ResponseParam;
	//UKismetSystemLibrary::DrawDebugLine(GetWorld(),VectorStart,VectorEnd,FColor::Red,5,13);
	bool SuccessHit = GetWorld()->LineTraceSingleByChannel(HitResultTemp, VectorStart, VectorEnd,
		ECollisionChannel::ECC_WorldStatic, QueryParams, ResponseParam);
	//UE_LOG(LogTemp, Warning, TEXT("Player Character: line trace done."));
	if (!SuccessHit) return false;
	//if (!IsValid(HitResult.GetComponent())) return false;
	UStaticMeshComponent* OtherMeshComp = Cast<UStaticMeshComponent>(HitResultTemp.GetComponent());
	//HitResult.
	if (OtherMeshComp == nullptr)return false;
	//UE_LOG(LogTemp, Warning, TEXT("Player Character: other mesh component name: %s."), *HitResultTemp.GetComponent()->GetName());
	if (OtherMeshComp->Mobility == EComponentMobility::Movable) return false;
	UStaticMeshComponent* StMeshC = Cast<UStaticMeshComponent>(OtherMeshComp);
	if (StMeshC == nullptr)return false;
	//UE_LOG(LogTemp, Warning, TEXT("Player Character: other mesh valid."));
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), StMeshC->Bounds.Origin, StMeshC->Bounds.BoxExtent, FColor::Red, FRotator(0, 0, 0), 12, 12);
	FVector BoxSize = StMeshC->Bounds.BoxExtent * 2;
	FVector SpawnLocation = HitResultTemp.Normal.GetAbs();
	int8 WitchSideDontMatter = 3;
	if (SpawnLocation.X > SpawnLocation.Y && SpawnLocation.X > SpawnLocation.Z)WitchSideDontMatter = 0;
	else if (SpawnLocation.Y > SpawnLocation.X && SpawnLocation.Y > SpawnLocation.Z)WitchSideDontMatter = 1;
	else if (SpawnLocation.Z > SpawnLocation.Y && SpawnLocation.Z > SpawnLocation.X)WitchSideDontMatter = 2;
	FString TempString = HitResultTemp.Normal.ToString();
	//UE_LOG(LogTemp,Warning,TEXT("Player Character: side that dose not matter: %i normal vector: %s"), WitchSideDontMatter,*TempString);
	float MinX = 0, MaxX = 0, MinY = 0, MaxY = 0, MinZ = 0, MaxZ = 0, TempFloat = 0;
	SpawnLocation = HitResultTemp.Location;

	TArray <FVector> Directions;
	FVector TraceStart = HitResultTemp.Normal + HitResultTemp.Location;
	TArray <float> OutFloats;
	FVector TempVector;



	switch (WitchSideDontMatter) {
	case 0:
		if (BoxSize.Y < PortalWidth || BoxSize.Z < PortalHeight) {
			UE_LOG(LogTemp, Warning, TEXT("Player Character: box too small: %s"), *BoxSize.ToString());
			return false;
		}
		Directions = { FVector(0,-1000,0), FVector(0,1000,0), FVector(0,0,-1000), FVector(0,0,1000) };
		OutFloats = CheckNearPortalObstacles(Directions, TraceStart);

		MinY = StMeshC->Bounds.Origin.Y - BoxSize.Y / 2;
		if (TraceStart.Y - OutFloats[0] > MinY)MinY = TraceStart.Y - OutFloats[0];
		MaxY = StMeshC->Bounds.Origin.Y + BoxSize.Y / 2;
		if (TraceStart.Y + OutFloats[1] < MaxY)MaxY = TraceStart.Y + OutFloats[1];
		MinZ = StMeshC->Bounds.Origin.Z - BoxSize.Z / 2;
		if (TraceStart.Z - OutFloats[2] > MinZ)MinZ = TraceStart.Z - OutFloats[2];
		MaxZ = StMeshC->Bounds.Origin.Z + BoxSize.Z / 2;
		if (TraceStart.Z + OutFloats[3] < MaxZ)MaxZ = TraceStart.Z + OutFloats[3];


		if (MaxY - MinY < PortalWidth || MaxZ - MinZ < PortalHeight) {
			UE_LOG(LogTemp, Warning, TEXT("Player Character: portal dos not fit."));
			return false;
		}

		TempFloat = HitResultTemp.Location.Y;
		SpawnLocation.Y = FMath::Clamp(TempFloat, MinY + PortalWidth / 2, MaxY - PortalWidth / 2);
		TempFloat = HitResultTemp.Location.Z;
		SpawnLocation.Z = FMath::Clamp(TempFloat, MinZ + PortalHeight / 2, MaxZ - PortalHeight / 2);

		break;
	case 1:
		if (BoxSize.X < PortalWidth || BoxSize.Z < PortalHeight) {
			UE_LOG(LogTemp, Warning, TEXT("Player Character: box too small: %s"), *BoxSize.ToString());
			return false;
		}
		Directions = { FVector(-1000,0,0), FVector(1000,0,0), FVector(0,0,-1000), FVector(0,0,1000) };
		OutFloats = CheckNearPortalObstacles(Directions, TraceStart);
		MinX = StMeshC->Bounds.Origin.X - BoxSize.X / 2;
		if (TraceStart.X - OutFloats[0] > MinX)MinX = TraceStart.X - OutFloats[0];
		MaxX = StMeshC->Bounds.Origin.X + BoxSize.X / 2;
		if (TraceStart.X + OutFloats[1] < MaxX)MaxX = TraceStart.X + OutFloats[1];
		MinZ = StMeshC->Bounds.Origin.Z - BoxSize.Z / 2;
		if (TraceStart.Z - OutFloats[2] > MinZ)MinZ = TraceStart.Z - OutFloats[2];
		MaxZ = StMeshC->Bounds.Origin.Z + BoxSize.Z / 2;
		if (TraceStart.Z + OutFloats[3] < MaxZ)MaxZ = TraceStart.Z + OutFloats[3];

		if (MaxX - MinX < PortalWidth || MaxZ - MinZ < PortalHeight) {
			UE_LOG(LogTemp, Warning, TEXT("Player Character: portal dos not fit."));
			return false;
		}
		TempFloat = HitResultTemp.Location.X;
		SpawnLocation.X = FMath::Clamp(TempFloat, MinX + PortalWidth / 2, MaxX - PortalWidth / 2);
		TempFloat = HitResultTemp.Location.Z;
		SpawnLocation.Z = FMath::Clamp(TempFloat, MinZ + PortalHeight / 2, MaxZ - PortalHeight / 2);
		break;
	case 2:
		if (BoxSize.Y < PortalWidth || BoxSize.X < PortalHeight) {
			UE_LOG(LogTemp, Warning, TEXT("Player Character: box too small: %s"), *BoxSize.ToString());
			return false;
		}
		Directions = { FVector(-1000,0,0), FVector(1000,0,0), FVector(0,-1000,0), FVector(0,1000,0) };
		OutFloats = CheckNearPortalObstacles(Directions, TraceStart);

		MinX = StMeshC->Bounds.Origin.X - BoxSize.X / 2;
		if (TraceStart.X - OutFloats[0] > MinX)MinX = TraceStart.X - OutFloats[0];
		MaxX = StMeshC->Bounds.Origin.X + BoxSize.X / 2;
		if (TraceStart.X + OutFloats[1] < MaxX)MaxX = TraceStart.X + OutFloats[1];
		MinY = StMeshC->Bounds.Origin.Y - BoxSize.Y / 2;
		if (TraceStart.Y - OutFloats[2] > MinY)MinY = TraceStart.Y - OutFloats[2];
		MaxY = StMeshC->Bounds.Origin.Y + BoxSize.Y / 2;
		if (TraceStart.Y + OutFloats[3] < MaxY)MaxY = TraceStart.Y + OutFloats[3];

		if (MaxY - MinY < PortalWidth || MaxX - MinX < PortalHeight) {
			UE_LOG(LogTemp, Warning, TEXT("Player Character: portal do not fit."));
			return false;
		}
		TempFloat = HitResultTemp.Location.Y;
		SpawnLocation.Y = FMath::Clamp(TempFloat, MinY + PortalWidth / 2, MaxY - PortalWidth / 2);
		TempFloat = HitResultTemp.Location.X;
		SpawnLocation.X = FMath::Clamp(TempFloat, MinX + PortalHeight / 2, MaxX - PortalHeight / 2);

		break;
	case 3: return false;
	}

	FRotator SpawnRotation = HitResultTemp.Normal.Rotation();
	TArray <APortalActor*> TempPortals = GameState->Portals;
	TempPortals.Remove(bOrange ? OrangePortal : BluePortal);
	//if (IsValid(bOrange ? BluePortal : OrangePortal)) { TempPortals.AddUnique( Cast<APortalActor>(bOrange ? BluePortal : OrangePortal));		}
	for(APortalActor* TempPortal : TempPortals){
		if (IsValid(TempPortal)) {
			FVector ToOtherPortal = TempPortal->GetActorLocation() - SpawnLocation;
			if (ToOtherPortal.Size() < PortalHeight) {
				float Distance = ToOtherPortal.Size();
				ToOtherPortal.Normalize();
				TempFloat = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, SpawnRotation.RotateVector(FVector(0, 0, 1)));
				float TempFloat2 = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, TempPortal->GetActorUpVector());

				//UE_LOG(LogTemp, Warning, TEXT("Player Character: dot products Z: %s + %s to %s."),				*FString::SanitizeFloat(TempFloat), *FString::SanitizeFloat(TempFloat2), *FString::SanitizeFloat(Distance / PortalHeight));

				if (abs(TempFloat) + abs(TempFloat2) > Distance * 2 / PortalHeight) return false;

				TempFloat = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, SpawnRotation.RotateVector(FVector(0, 1, 0)));
				TempFloat2 = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, TempPortal->GetActorRightVector());
				//UE_LOG(LogTemp, Warning, TEXT("Player Character: dot products Y: %s + %s to %s."),				*FString::SanitizeFloat(TempFloat), *FString::SanitizeFloat(TempFloat2), *FString::SanitizeFloat(Distance / PortalWidth));
				if (abs(TempFloat) + abs(TempFloat2) > Distance * 2 / PortalWidth) return false;

				TempFloat = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, HitResultTemp.Normal);
				TempFloat2 = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, TempPortal->GetActorForwardVector());
				//UE_LOG(LogTemp, Warning, TEXT("Player Character: dot products X: %s + %s to %s."),				*FString::SanitizeFloat(TempFloat), *FString::SanitizeFloat(TempFloat2), *FString::SanitizeFloat(Distance / PortalHeight));
				if (abs(TempFloat) + abs(TempFloat2) > Distance / 40) return false;
			}
		}
	}
	/*
	AActor* TempActor = bOrange ? BluePortal : OrangePortal;
	if (IsValid(TempActor)) {
		FVector ToOtherPortal = TempActor->GetActorLocation() - SpawnLocation;
		if (ToOtherPortal.Size() < PortalHeight) {
			float Distance = ToOtherPortal.Size();
			ToOtherPortal.Normalize();
			TempFloat = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, SpawnRotation.RotateVector(FVector(0,0,1))) ;
			float TempFloat2 = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, TempActor->GetActorUpVector());

			//UE_LOG(LogTemp, Warning, TEXT("Player Character: dot products Z: %s + %s to %s."),				*FString::SanitizeFloat(TempFloat), *FString::SanitizeFloat(TempFloat2), *FString::SanitizeFloat(Distance / PortalHeight));

			if (abs(TempFloat) + abs(TempFloat2) > Distance*2 / PortalHeight) return false;

			TempFloat = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, SpawnRotation.RotateVector(FVector(0, 1, 0)));
			TempFloat2 = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, TempActor->GetActorRightVector());
			//UE_LOG(LogTemp, Warning, TEXT("Player Character: dot products Y: %s + %s to %s."),				*FString::SanitizeFloat(TempFloat), *FString::SanitizeFloat(TempFloat2), *FString::SanitizeFloat(Distance / PortalWidth));
			if (abs(TempFloat )+ abs(TempFloat2 )> Distance*2 / PortalWidth) return false;

			TempFloat = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, HitResultTemp.Normal);
			TempFloat2 = UKismetMathLibrary::Dot_VectorVector(ToOtherPortal, TempActor->GetActorForwardVector());
			//UE_LOG(LogTemp, Warning, TEXT("Player Character: dot products X: %s + %s to %s."),				*FString::SanitizeFloat(TempFloat), *FString::SanitizeFloat(TempFloat2), *FString::SanitizeFloat(Distance / PortalHeight));
			if (abs(TempFloat) + abs(TempFloat2) > Distance / 40) return false;
		}
	}
	*/
	bool TempBool = GetWorld()->LineTraceSingleByChannel(HitResultTemp,SpawnLocation+HitResultTemp.Normal, SpawnLocation + HitResultTemp.Normal*125,
		ECollisionChannel::ECC_WorldStatic, QueryParams, FCollisionResponseParams::DefaultResponseParam);
	if (TempBool)return false;


	Server_SpawnPortal(bOrange,SpawnLocation,SpawnRotation, PlayerController);
	if (FireAnimation && HandsAnimationsInstance && !HasAuthority())HandsAnimationsInstance->Montage_Play(FireAnimation);
	return true;
	FActorSpawnParameters SpawnParamsTemp;
	APortalActor* TempPortal =Cast<APortalActor>(	GetWorld()->SpawnActor<AActor>(PortalClass, SpawnLocation,SpawnRotation , SpawnParamsTemp)	);
	if (TempPortal) {
		//UE_LOG(LogTemp, Warning, TEXT("Player Character: Portal spawned."));
		if (FireAnimation && HandsAnimationsInstance)HandsAnimationsInstance->Montage_Play(FireAnimation);
		if (bOrange) { 
			if (OrangePortal)OrangePortal->ClosePortal();
			TempPortal->StartSetting(true,BluePortal, PlayerController);
			OrangePortal = TempPortal;
		}
		else {
			if (BluePortal)BluePortal->ClosePortal();
			TempPortal->StartSetting(false,OrangePortal,PlayerController);
			BluePortal = TempPortal;
		}
	}
	else UE_LOG(LogTemp, Warning, TEXT("Player Character: Portal did not spawned."));

	return true;
}

void ATeleportProjectCharacter::Server_SpawnPortal_Implementation(bool bOrange, FVector SpawnLocation, FRotator SpawnRotation, APlayerController* PlController)
{
	FActorSpawnParameters SpawnParamsTemp;
	APortalActor* TempPortal = Cast<APortalActor>(
		GetWorld()->SpawnActor<AActor>(PortalClass, SpawnLocation, SpawnRotation, SpawnParamsTemp));
	if (TempPortal) {
		//UE_LOG(LogTemp, Warning, TEXT("Player Character: Portal spawned."));
		TempPortal->PortalsDelegateList.AddDynamic(this, &ATeleportProjectCharacter::AnotherTestDelegateFunction);
		if (FireAnimation && HandsAnimationsInstance)HandsAnimationsInstance->Montage_Play(FireAnimation);
		if (bOrange) {
			if (OrangePortal)OrangePortal->ClosePortal();
			TempPortal->StartSetting(true, BluePortal, PlController);
			OrangePortal = TempPortal;
		}
		else {
			if (BluePortal)BluePortal->ClosePortal();
			TempPortal->StartSetting(false, OrangePortal, PlController);
			BluePortal = TempPortal;
		}
	}
}

TArray <float> ATeleportProjectCharacter::CheckNearPortalObstacles( TArray<FVector> TraceDirections, FVector TraceStart)
{
	TArray <float> OutFloats ;
	int8 i = 0;
	bool TempBool = false;
	FHitResult TempHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	for (FVector Direction : TraceDirections){
		TempBool = GetWorld()->LineTraceSingleByChannel(TempHit, TraceStart,TraceStart+TraceDirections[i],ECollisionChannel::ECC_WorldStatic,
						QueryParams,FCollisionResponseParams::DefaultResponseParam);

		//DrawDebugLine(GetWorld(), TraceStart, TempBool ? TempHit.Location : TraceStart + TraceDirections[i], FColor::Red, false, 10);
		if (TempBool) {			OutFloats.Insert(TempHit.Distance, i); 		}
		else OutFloats.Insert( TraceDirections[i].Size(),i);
		//UE_LOG(LogTemp, Warning, TEXT("Player Character: line traced %i  :  vector: %s."),i,*TraceDirections[i].ToString());
		i++;
	}
	return OutFloats;
}
