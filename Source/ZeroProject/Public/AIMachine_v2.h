// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIMachine_v2.generated.h"

UCLASS()
class ZEROPROJECT_API AAIMachine_v2 : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AAIMachine_v2();

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
        float AccelerationRate = 100.0f;
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
        float BoostConsumption = 0.1f;
    UPROPERTY(EditAnywhere)
        float Steering = 0.5f;
    UPROPERTY(EditAnywhere)
        float ShieldDamage = 0.1f;
    UPROPERTY(EditAnywhere)
        float Energy = 1.f;
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

    UPROPERTY(BlueprintReadWrite)
        bool bCanRace = false;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
        float Speed = 0.f;

    int Gravity = 9800;
    float SpeedModifier = 0.f;
    float VerticalSpeed = 0;
    float LastVerticalDelta;
    float LastHeight;
    float DeltaDeltaX;
    float TempDeltaX;
    float LastDeltaX;
    bool bCanSetNewDesiredDeltaX = false;
    bool bGrounded = true;
    bool bBouncing = false;
    FVector LastDirection;
    FVector LastSurfaceLocation;
    FVector DesiredLocation;
    FRotator DesiredRotation;
    FVector DeltaLocationX;
    FVector DeltaLocationY;
    FVector HitDelta;
    FVector LocalDeltaXY;
    FTimerHandle TimerHandle;
    FTimerDelegate TimerDelegate;

    UFUNCTION()
        void StartRace();
    UFUNCTION(BlueprintCallable)
        void Init();

    void SetOrientation();
    void Drive(float deltaTime);
    void Move(float deltaTime);
    void CalculateOnRailSpeed(float deltaTime);

public:
    UPROPERTY(BlueprintReadWrite)
        float PreSpeed = 0.f;
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
        float CoveredDistance = 0.0f;

    float DesiredDeltaX;
    FVector RealDeltaLocation;

    void CheckDepletion();

};
