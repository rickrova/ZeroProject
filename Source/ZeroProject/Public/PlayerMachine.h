// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TrackManager.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerMachine.generated.h"

UCLASS()
class ZEROPROJECT_API APlayerMachine : public APawn
{
	GENERATED_BODY()

public:
	APlayerMachine();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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
		float BoostDeccelerationRate = 500;
	UPROPERTY(EditAnywhere)
		float Boost = 2500.f;
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
		float Steering = 100;
	UPROPERTY(VisibleAnywhere)//remove visibility
		int CurrentSegment;
	UPROPERTY(VisibleAnywhere)//remove visibility
		int Rank;
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* CollisionComponent;
	UPROPERTY(VisibleAnywhere)
		UArrowComponent* ArrowComponent;
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;
	UPROPERTY(EditAnywhere)
		UCameraComponent* CameraComponent;
	UPROPERTY(VisibleAnywhere)
		UArrowComponent* ArrowComponentDos;

	//Debug purpose
	UPROPERTY(VisibleAnywhere)//remove visibility
		float RawProgress;
	UPROPERTY(VisibleAnywhere)//remove visibility
		float NetProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)//remove visibility
		UTrackManager* TrackManager;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)//remove visibility
		ASplineActor* Spline;

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
	float SpeedModifier = 0;
	float BoostModifier = 0;
	int Lap;
	FVector BounceDirection;
	FVector TrackDirection;
	FVector MachineDirection;
	//FVector LastMachineDirection;
	FVector SurfaceNormal;
	FVector SurfacePoint;
	//FVector AvoidDirection;
	FVector ClosestSplinePoint;
	FRotator VisibleRotation;
	FRotator CameraRotation;
	FVector CameraLocation;
	bool bGrounded;
	bool bCanFindSurface;
	bool bDepleted;
	bool bInstantSteer;

	State MachineState;
	UPROPERTY(VisibleAnywhere)
		int INTState;

	FVector MovementInput;
	bool bAccelerating;

	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void Accelerate();
	void Deccelerate();
	void ExpelBoost();

	void ComputeMovement(float deltaTime);
	void AlignToSurface(float deltaTime);
	void ComputeClosestSurfaceNormal(float deltaTime);
	void Bounce(FHitResult* hit, FVector deltaLocation);
	void ComputeDirection(float deltaTime);
	void CheckTrackProgress();
	void CheckAvoidables(float deltaTime);
	void CheckForAlterPath();
	void CheckForBoost();
	void SoftDestroy(FString inText);
	void CheckStuck(float deltaTime, FVector initialLocation);
	void UpdateVisibleRotation(float deltaTime);

public:
	FVector LastDeltaLocation;
	float Mass = 10.f;
	bool bStucked;
	int ID;
	AAdaptativeMachine* CurrentCollisionMachine;

	FVector Push(FVector pushVelocity, bool bCalculateDamage);
	void SetupTrackManager(UTrackManager* inTrackManager, int inID);
	void SetRank(int inRank);
};
