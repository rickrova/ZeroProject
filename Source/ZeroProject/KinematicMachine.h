// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "KinematicMachine.generated.h"

UCLASS()
class ZEROPROJECT_API AKinematicMachine : public APawn
{
	GENERATED_BODY()

public:
	AKinematicMachine();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere)
		class USphereComponent* KinematicComponent;

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;

	UPROPERTY(VisibleAnywhere)
		class UArrowComponent* ArrowComponent;

	UPROPERTY(VisibleAnywhere)
		class UArrowComponent* CameraContainerComponent;

	UPROPERTY(EditAnywhere)
		class USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere)
		class UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere)
		float MaxSpeed = 5000.0f;
    UPROPERTY(EditAnywhere)
        float BoostSpeed = 5000.0f;
	UPROPERTY(EditAnywhere)
		float BoostDecceleration = 10.0f;
	UPROPERTY(EditAnywhere)
		float AccelerationRate = 5000.0f;
	UPROPERTY(EditAnywhere)
		float DeccelerationRate = 5000.0f;
	UPROPERTY(EditAnywhere)
		float Gravity = 980.f;
	UPROPERTY(EditAnywhere)
		float RayDistance = 1.0f;
	UPROPERTY(EditAnywhere)
		float MinSteering = 100.0f;
	UPROPERTY(EditAnywhere)
		float MaxSteering = 150.0f;
	UPROPERTY(EditAnywhere)
		float DriftThereshold = 3.f;
	UPROPERTY(EditAnywhere)
		int Raycount = 10;
	UPROPERTY(EditAnywhere)
		float RaysOffset = 20;
	UPROPERTY(EditAnywhere)
		float RaySetOffset = 100;
	UPROPERTY(EditAnywhere)
		float RaySetVerticalOfset = 200;
	UPROPERTY(EditAnywhere)
		float RaySetDistance = 500;

	FVector GravityDirection = -FVector::UpVector;

	//Input variables
	float Steering = 100.0f;
	float Speed = 0.f;
    float SpeedModifier = 0.f;
	float VerticalSpeed = 0;
	float VerticalSpeedModifier = 1;
	float RightDrift = 0;
	float LeftDrift = 0;
	float AirYaw = 0;
    float AirPitch = 0;
	float DriftYaw = 0;
	float DriftingOffset = 0.f;
	FVector MovementInput;
	FVector RawMovementInput;
	FVector LastMachineLocation;
	bool bAccelerating = false;
	bool bBraking = false;
	bool bPendingAcceleration = false;
	bool bGrounded = false;
	bool bDrifting = false;
	bool bPendingDrift = false;
	bool bCanDrift = true;
	bool bCanExitDrift = false;
	FTimerHandle TimerHandle;
	FTimerHandle EDTimerHandle;
	FTimerDelegate TimerDel;
	FTimerDelegate DriftDelegate;
	FTimerDelegate ExitDriftDelegate;

	//Input functions
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void GripLeft(float AxisValue);
	void GripRight(float AxisValue);
	void DriftLeft();
	void DriftRight();
	void Accelerate();
	void Deccelerate();
	void Brake();
	void LiftBrake();
    void Boost();
	void Raycast(float deltaTime);

	UFUNCTION()
		void ExitDrift();
	UFUNCTION()
		void ResetDrift();
	UFUNCTION()
		void ResetExitDrift();
};
