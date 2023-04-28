// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineActor.h"
#include "TrackManager.h"
#include "AdaptativeMachine.generated.h"

UCLASS()
class ZEROPROJECT_API AAdaptativeMachine : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAdaptativeMachine();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere)
		float AccelerationRate = 20000;
	UPROPERTY(EditAnywhere)
		float MaxSpeed = 10000.f;
	UPROPERTY(EditAnywhere)
		float BoostDeccelerationRate = 5000;
	UPROPERTY(EditAnywhere)
		float Boost = 2500.f;
	UPROPERTY(EditAnywhere)
		float TraceUpDistance = 170.f;
	UPROPERTY(EditAnywhere)
		float TraceDownDistance = 200.f;
	UPROPERTY(EditAnywhere)
		float DistanceToSurface = 20;
	UPROPERTY(EditAnywhere)
		int SurfaceAtractionForce = 9800;
	UPROPERTY(EditAnywhere)
		float BounceDeccelerationRate = 10000;
	UPROPERTY(EditAnywhere)
		float FakeBounceSpeed = 200;
	UPROPERTY(EditAnywhere)
		float SideBounceDeviationAngle = 45;
	UPROPERTY(EditAnywhere)
		float Steering = 10;
	UPROPERTY(VisibleAnywhere)//remove visibility
		int CurrentSegment;
	UPROPERTY(VisibleAnywhere)//remove visibility
		int Rank;
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* CollisionComponent;
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;

	//Debug purpose
	UPROPERTY(VisibleAnywhere)//remove visibility
		float RawProgress;
	UPROPERTY(VisibleAnywhere)//remove visibility
		float NetProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)//remove visibility
		UTrackManager* TrackManager;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)//remove visibility
		ASplineActor* Spline;

	float Speed;
	float BoostSpeed;
	float SurfaceAtractionSpeed;
	float BounceSpeed;
	float CurrentYaw;
	float RealSpeed;
	float DesiredDetourAngle;
	float CurrentDetourAngle;
	float NormalizedDesiredAvoidAmount;
	float NormalizedCurrentAvoidAmount;
	float VisibleRotationSpeed;
	float AlterPathTime;
	float CurrentAlterPathTimeThreshold;
	float BoostTime;
	float CurrentBoostTimeThreshold;
	UPROPERTY(VisibleAnywhere)
	float Condition;
	float StuckedTime;
	int Lap;
	FVector BounceDirection;
	FVector TrackDirection;
	FVector MachineDirection;
	//FVector LastMachineDirection;
	FVector SurfaceNormal;
	FVector SurfacePoint;
	FVector AvoidDirection;
	//FVector ClosestSplinePoint;
	FRotator VisibleRotation;
	bool bGrounded;
	bool bDetourAvailable;
	bool bOnDetour;
	bool bCanFindSurface;
	bool bDepleted;
	
	void ComputeMovement(float deltaTime);
	void AlignToSurface(float deltaTime);
	void ComputeClosestSurfaceNormal(float deltaTime);
	void Bounce(FHitResult* hit, FVector deltaLocation);
	void ComputeDirection(float deltaTime);
	void CheckTrackProgress();
	void CheckAvoidables(float deltaTime);
	void CheckForAlterPath();
	void CheckForBoost();
	void SoftDestroy();
	void CheckStuck(float deltaTime, FVector initialLocation);
	void UpdateVisibleRotation(float deltaTime);

	UFUNCTION()
	void SetDetour();

public:
	FVector LastDeltaLocation;
	float Mass = 10.f;
	bool bStucked;
	int ID;

	FVector Push(FVector pushVelocity, bool bCalculateDamage);
	void SetupTrackManager(UTrackManager* inTrackManager, int inID);
	void SetRank(int inRank);
};
