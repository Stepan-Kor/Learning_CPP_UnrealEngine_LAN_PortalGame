// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PortalActor.h"
#include "GameFramework/Character.h"
#include "TeleportProjectCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class ATeleportProjectCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

	/** Gun mesh: VR view (attached to the VR controller directly, no arm, just the actual gun) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* VR_Gun;

	/** Location on VR gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* VR_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Motion controller (right hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* R_MotionController;

	/** Motion controller (left hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* L_MotionController;

public:
	ATeleportProjectCharacter();
	UFUNCTION() void AnotherTestDelegateFunction(int IntegerArgument) ;

protected:
	virtual void BeginPlay();

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class ATeleportProjectProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint8 bUsingMotionControllers : 1;

protected:
	
	/** Fires a projectile. */
	void OnFire();

	/** Resets HMD orientation and position in VR. */
	void OnResetVR();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	struct TouchData
	{
		TouchData() { bIsPressed = false;Location=FVector::ZeroVector;}
		bool bIsPressed;
		ETouchIndex::Type FingerIndex;
		FVector Location;
		bool bMoved;
	};
	void BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location);
	TouchData	TouchItem;
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* 
	 * Configures input for touchscreen devices if there is a valid touch interface for doing so 
	 *
	 * @param	InputComponent	The input component pointer to bind controls to
	 * @returns true if touch controls were enabled.
	 */
	virtual void GetLifetimeReplicatedProps(TArray <FLifetimeProperty> & OutLifetimeProps) const override;
	bool EnableTouchscreenMovement(UInputComponent* InputComponent);
	bool SpawnPortal(bool bOrange);
	const float PortalWidth =105;
	const float PortalHeight = 225;
	
	TArray <float> CheckNearPortalObstacles( TArray<FVector>TraceDirections, FVector TraceStart);
	//void SetMinMaxPortalAxis(float& Min, float& Max);

	void OpenMenu();
	UFUNCTION(Server, Reliable) void Server_ChangeMovementSpeed(float Value);
	void Server_ChangeMovementSpeed_Implementation(float Value);
	UFUNCTION(NetMulticast, Reliable) void Multi_ChangeMovementSpeed(float Value);
	void Multi_ChangeMovementSpeed_Implementation(float Value);

	UPROPERTY( ReplicatedUsing = UpdateCamera) float CameraPitch;
	UFUNCTION() void UpdateCamera();
	UFUNCTION(Server, Reliable) void Server_UpdateCameraPitch(float Value);
	void Server_UpdateCameraPitch_Implementation(float Value);
	void BallShot(bool Blank=false);
	UFUNCTION(Server, Reliable) void Server_BallShot();
	void Server_BallShot_Implementation();
	UFUNCTION(NetMultiCast, Reliable) void Multi_BallShot();
	void Multi_BallShot_Implementation();
	UFUNCTION(NetMultiCast, Reliable) void Multi_TurnController(FRotator NewRotation, FVector NewVelocity);
	void Multi_TurnController_Implementation(FRotator NewRotation, FVector NewVelocity);
	UFUNCTION(Server, Reliable) void Server_SpawnPortal(bool bOrange, FVector SpawnLocation, FRotator SpawnRotation, APlayerController* PlController);
	void Server_SpawnPortal_Implementation(bool bOrange, FVector SpawnLocation, FRotator SpawnRotation, APlayerController* PlController);
	void LookUpCustom(float Value);
	bool CameraUpdateReady=true;
	virtual void Tick(float DeltaTime) override;
	APortalsGameState* GameState;

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly) TSubclassOf<APortalActor>PortalClass= APortalActor::StaticClass();
	UPROPERTY(EditDefaultsOnly) TSubclassOf<class UHUD_Widget>HUDWidgetClass;
	UPROPERTY() class UHUD_Widget* HUDWidget;
	UFUNCTION() void ShotOrangePortal();
	UFUNCTION() void ShotBluePortal();
	UFUNCTION() void RunStart();
	UFUNCTION() void RunEnd();
	UFUNCTION() void TeleportPlayerCharacter(APortalActor* TargetPortal, APortalActor* EnterPortal);
	APortalActor* OrangePortal;
	APortalActor* BluePortal;
	APlayerController* PlayerController;


	UAnimInstance* HandsAnimationsInstance;
};