// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimeLineComponent.h"
#include "PortalActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSomeNewDelegate, int, IntegerParam);

UCLASS()
class TELEPORTPROJECT_API APortalActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalActor();
	virtual void Destroyed()override;
	UFUNCTION(BlueprintNativeEvent) void SomeVirtualFunctionForTest();
	void SomeVirtualFunctionForTest_Implementation();
	UFUNCTION() void SomeTestForDelegateFunction(int SomeIntArgument);
	UPROPERTY (BlueprintAssignable, Category="Event dispatcher")FSomeNewDelegate PortalsDelegateList;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlapedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool FromSweep, const FHitResult& HitResut);
	UMaterialInstanceDynamic* PortalMaterialInstance;
	class APortalsGameState* PortalsGameState;

	bool CurrentlyIgnoringActor(AActor* Suspect);
	/*
	UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, 
		bool, bFromSweep, const FHitResult&, SweepResult);*/
public:
	bool bOrange;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(EditDefaultsOnly) UParticleSystem* SparksEmitter;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Category = "Mesh") UStaticMeshComponent* Mesh;
	UFUNCTION() void StartSetting(bool bIsOrange, APortalActor* SecondPortal, APlayerController* Controller);
	UFUNCTION() void ClosePortal();
	UFUNCTION() void IgnoreThisActor(AActor* IgnoreThis);
	TMap <AActor*, float> IgnoreThisActors;

	UPROPERTY(EditDefaultsOnly) USoundBase* SpawnSoundOrange;
	UPROPERTY(EditDefaultsOnly) USoundBase* SpawnSoundBlue;
	APortalActor* AnotherPortal;
	

protected:
	UFUNCTION()void TimeLineUpdate(float Value);
	FTimeline TimelineOpenPortal;
	FOnTimelineFloat OpenPortalFloatProgress;
	UPROPERTY(EditDefaultsOnly)UCurveFloat* CurveOpenPortal;
	const float DestructionTimeMax = 0.2f;
	float DestructionTimeLeft = DestructionTimeMax;
	UPROPERTY(Replicated)bool bPortalDestroingSelf=false;
	FColor PortalColor=FColor(0,0,0);
	UFUNCTION(NetMultiCast, Reliable)void Multi_ChangeColor(bool Orange, APortalActor* SecondPortal, FColor NewColor, APlayerController* Controller);
	void Multi_ChangeColor_Implementation(bool Orange, APortalActor* SecondPortal, FColor NewColor, APlayerController* Controller);
public:
	virtual void GetLifetimeReplicatedProps(class TArray<FLifetimeProperty> &OutLifetimeProps)const;

};
