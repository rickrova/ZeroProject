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
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UTrackManager* TrackManager;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		ASplineActor* Spline;

	float Speed;
	float SurfaceAtractionSpeed;
	float BounceSpeed;
	float CurrentYaw;
	float RealSpeed;
	float DesiredDetourAngle;
	float CurrentDetourAngle;
	int CompletedSegments;
	FVector BounceDirection;
	FVector SurfaceNormal;
	FVector SurfacePoint;
	FVector LastActorLocation;
	//FVector ClosestSplinePoint;
	bool bGrounded;
	bool bDetourAvailable;
	bool bOnDetour;
	
	void ComputeMovement(float deltaTime);
	void AlignToSurface(float deltaTime);
	void ComputeClosestSurfaceNormal(float deltaTime);
	void Bounce(FHitResult* hit);
	void ComputeDirection(float deltaTime);
	void CheckSplineProgress();

	UFUNCTION()
	void SetDetour();

public:

	void Push(FVector bounceDirection, float bounceSpeed);
};
