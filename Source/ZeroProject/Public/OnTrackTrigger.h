// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "OnTrackTrigger.generated.h"

UCLASS()
class ZEROPROJECT_API AOnTrackTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOnTrackTrigger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere)
		bool bBoost = false;
	UPROPERTY(EditAnywhere)
		bool bSlow = false;
	UPROPERTY(EditAnywhere)
		bool bEnergyModifier = false;
	UPROPERTY(EditAnywhere)
		bool bJump = false;
	UPROPERTY(EditAnywhere)
		float BoostRatio = 0.75f;
	UPROPERTY(EditAnywhere)
		float SlowRatio = 0.75f;
	UPROPERTY(EditAnywhere)
		float EnergyRatio = 0.1f;
	UPROPERTY(EditAnywhere)
		float JumpRatio = 100.f;
	UPROPERTY(EditAnywhere)
		float MagneticRatio = 100.f;
	UPROPERTY(VisibleAnywhere)
		UBoxComponent* Trigger;
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;

	void BoostBehavior(AActor *);
	void JumpBehavior(AActor*);
	void SlowBehavior(AActor*);
	void ResetSlowBehavior(AActor*);
	void StartEnergyTransmission(AActor*);
	void EndEnergyTransmission(AActor*);

	UFUNCTION()
		void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
