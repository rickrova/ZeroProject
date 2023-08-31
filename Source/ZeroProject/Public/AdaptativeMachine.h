// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineActor.h"
#include "TrackManager_v2.h"
#include "Components/ArrowComponent.h"
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

	enum State {
		STRAIGHT,
		DRIVING,
		DEVIATION,
		EVADING,
	};

	UPROPERTY(EditAnywhere)
		float AccelerationRate = 10000;
	UPROPERTY(EditAnywhere)
		float MaxSpeed = 20000.f;
	UPROPERTY(EditAnywhere)
		float BoostAccelerationRate = 5000;
	UPROPERTY(EditAnywhere)
		float BoostDeccelerationRate = 500;
	UPROPERTY(EditAnywhere)
		float BoostDurationTime = 2.f;
	UPROPERTY(EditAnywhere)
		float TraceUpDistance = 400.f;
	UPROPERTY(EditAnywhere)
		float TraceDownDistance = 100.f;
	UPROPERTY(EditAnywhere)
		float DistanceToSurface = 20;
	UPROPERTY(EditAnywhere)
		int SurfaceAtractionForce = 9800;
	UPROPERTY(EditAnywhere)
		float BounceDeccelerationRate = 10000;
	UPROPERTY(EditAnywhere)
		float FakeBounceSpeed = 4000;
	UPROPERTY(EditAnywhere)
		float SideBounceDeviationAngle = 45;
	UPROPERTY(EditAnywhere)
		float Steering = 2.0;
	UPROPERTY(VisibleAnywhere)//remove visibility
		int CurrentSegment;
	UPROPERTY(VisibleAnywhere)//remove visibility
		int Rank;
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* CollisionComponent;
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;
	UPROPERTY(EditAnywhere)
		UArrowComponent* TargetOrientationArrowComponent;
	UPROPERTY(EditAnywhere)
		UArrowComponent* InitialOrientationArrowComponent;

	//Debug purpose
	UPROPERTY(VisibleAnywhere)//remove visibility
		float RawProgress;
	UPROPERTY(VisibleAnywhere)//remove visibility
		float NetProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)//remove visibility
		ATrackManager_v2* TrackManager;

	UPROPERTY(VisibleAnywhere)
	float Speed;
	UPROPERTY(VisibleAnywhere)
	float BoostSpeed;
	UPROPERTY(VisibleAnywhere)
	float SurfaceAtractionSpeed;
	UPROPERTY(VisibleAnywhere)
	float BounceSpeed;
	float CurrentYaw;
	float RealSpeed;
	float DesiredDetourAngle;
	float CurrentDetourAngle;
	UPROPERTY(VisibleAnywhere)
	float NormalizedDesiredAvoidAmount;
	UPROPERTY(EditAnywhere)
	float NormalizedDesiredDriveAmount;
	UPROPERTY(VisibleAnywhere)
	float NormalizedCurrentAvoidAmount;
	float VisibleRotationSpeed;
	float AlterPathTime;
	float CurrentAlterPathTimeThreshold;
	float RestorePathTime;
	float CurrentRestorePathTimeThreshold;
	float BoostTime;
	float CurrentBoostTimeThreshold;
	UPROPERTY(VisibleAnywhere)
	float Condition;
	float StuckedTime;
	UPROPERTY(VisibleAnywhere)
	float CurrentSteering;
	float DrivingSteering = 0.1f;
	float BoostRemainingTime = 0;
	float ExternalBoostRemainingTime = 0;
	float EnergyTransmissionRatio = 0;
	int Lap;
	FVector BounceDirection;
	FVector TrackDirection;
	FVector MachineDirection;
	FVector MagneticDirection;
	//FVector LastMachineDirection;
	FVector SurfaceNormal;
	FVector SurfacePoint;
	//FVector AvoidDirection;
	FVector ClosestSplinePoint;
	FRotator VisibleRotation;
	bool bGrounded;
	bool bCanFindSurface;
	bool bDepleted;
	bool bInstantSteer;
	bool bSignedIn;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanRace = false;
	bool bEnergyTransmission = false;
	bool bMagneticZone = false;
	bool bJumping = false;

	USplineComponent* MagneticSpline;

	State MachineState;
	UPROPERTY(VisibleAnywhere)
	int INTState;
	
	void ComputeMovement(float deltaTime);
	void AlignToSurface(float deltaTime);
	void ComputeClosestSurfaceNormal(float deltaTime);
	void Bounce(FHitResult* hit, FVector deltaLocation);
	void ComputeDirection(float deltaTime);
	void CheckTrackProgress();
	void CheckAvoidables(float deltaTime);
	void CheckForAlterPath();
	void CheckForBoost();
	void CheckEnergy(float);
	void SoftDestroy(FString inText);
	void CheckStuck(float deltaTime, FVector initialLocation);
	void UpdateVisibleRotation(float deltaTime);

public:
	FVector LastDeltaLocation;
	float Mass = 10.f;
	bool bStucked;
	int ID;
	AAdaptativeMachine* CurrentCollisionMachine;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)//remove visibility
		ASplineActor* Spline;

	FVector Push(FVector pushVelocity, bool bCalculateDamage);
	void SetupTrackManager(ATrackManager_v2* inTrackManager, int inID);
	void SetRank(int inRank);
	void Disable();
	void ExternalBoost(float);
	void Jump(float);
	void Slow(float);
	void ResetSlow(float);
	void StartEnergyTransmission(float);
	void EndEnergyTransmission();
	void StartMagnetic(USplineComponent*);
	void EndMagnetic();
};
