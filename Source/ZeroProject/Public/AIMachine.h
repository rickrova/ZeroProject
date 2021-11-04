// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIMachine.generated.h"

UCLASS()
class ZEROPROJECT_API AAIMachine : public AActor
{
	GENERATED_BODY()
	
public:	
	AAIMachine();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;
	UPROPERTY(VisibleAnywhere)
		class UArrowComponent* SurfaceComponent;
	UPROPERTY(VisibleAnywhere)
		class UArrowComponent* DynamicComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
        USkeletalMeshComponent* Guide;

	UPROPERTY(EditAnywhere)
		float MaxSpeed = 280.f;
    UPROPERTY(EditAnywhere)
        float AnimationTime = 0.f;
    UPROPERTY(EditAnywhere)
        float CurveLength = 0.f;
    UPROPERTY(EditAnywhere)
        float BoostSpeed = 140.f;
    UPROPERTY(EditAnywhere)
        float BoostChance = 0.f;
    UPROPERTY(EditAnywhere)
        float BoostDecceleration = 40.0f;
    UPROPERTY(EditAnywhere)
        float AccelerationRate = 100.0f;
    UPROPERTY(EditAnywhere)
        float Steering = 0.5f;
    UPROPERTY(EditAnywhere)
        float HitBounceScaler = 50.f;
    UPROPERTY(EditAnywhere)
        float HitDecceleration = 10.f;
    UPROPERTY(EditAnywhere)
        float DeltaX = 0.f;
	UPROPERTY(EditAnywhere)
		float DistanceToFloor = 20.f;
	UPROPERTY(EditAnywhere)
		float TraceOffset = 100.f;
	UPROPERTY(EditAnywhere)
		float ContinuityThereshold = 20.f;
	UPROPERTY(EditAnywhere)
		float SmartDeltaAngleTheresholdLow = 0.05f;
	UPROPERTY(EditAnywhere)
		float SmartDeltaAngleTheresholdHigh = 0.5f;
    UPROPERTY(EditAnywhere)
        float CurveFactor = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float StartPosition = 0.f;
    UPROPERTY(EditAnywhere)
    bool bDebug;

	UPROPERTY(BlueprintReadOnly)
		float Speed = 0.f;

    int Gravity = 9800;
    float SpeedModifier = 0.f;
    float VerticalSpeed = 0;
    float LastVerticalDelta;
    float LastHeight;
	bool bCanRace = false;
	bool bCanSetNewDesiredDeltaX = false;
    bool bGrounded = true;
    bool bBouncing = false;
    FVector LastDirection;
	FVector LastSurfaceLocation;
	FVector DesiredLocation;
	FRotator DesiredRotation;
    FVector HitDelta;
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;

	UFUNCTION()
		void StartRace();
    
    void SetHeight(float deltaTime);

public:
	UPROPERTY(BlueprintReadWrite)
		float PreSpeed = 0.f;
    UPROPERTY(BlueprintReadOnly)
        float CoveredDistance = 0.0f;
    
    float DesiredDeltaX = 0;
    FVector RealDeltaLocation;
    
    void HitByMachine(float rightDot);
	void HitByMachine2(float forwardDot);
    void HitByPlayer(FVector hitDelta, float deltaTime);
    
    void Bounce(FVector hitDirection, float hitMagnitude, bool external);
};
