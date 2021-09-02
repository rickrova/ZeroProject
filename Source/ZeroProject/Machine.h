// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Machine.generated.h"

UCLASS()
class ZEROPROJECT_API AMachine : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMachine();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere)
		class USphereComponent* PhysicsComponent;

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
		float Acceleration = 1.0f;
	UPROPERTY(EditAnywhere)
		float MaxSpeed = 1.0f;
	UPROPERTY(EditAnywhere)
		float GravityForce = 174270.0f;
	UPROPERTY(EditAnywhere)
		float RayDistance = 1.0f;
	UPROPERTY(EditAnywhere)
		float MinSteering = 100.0f;
	UPROPERTY(EditAnywhere)
		float MaxSteering = 150.0f;
	UPROPERTY(EditAnywhere)
		float MinPhysicsFactor = 0.75f;
	UPROPERTY(EditAnywhere)
		float VisibleComponentRotationFollowSpeed = 2.0f;
	UPROPERTY(EditAnywhere)
		UMaterialInstance* CeroFrictionMaterial;
	UPROPERTY(EditAnywhere)
		UMaterialInstance* SomeFrictionMaterial;
	UPROPERTY(EditAnywhere)
		UMaterialInstance* FullFrictionMaterial;

	FVector GravityDirection = -FVector::UpVector;

	//Input variables
	float Steering = 100.0f;
	float PhysicsFactor = 0.5f;
	FVector MovementInput;
	bool bAccelerating;
	bool bBraking;
	bool bPendingAcceleration;
	FVector LastContact;
	FVector LastNormal;
	FVector MachineLastPosition;

	//Input functions
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void DriftRight(float AxisValue);
	void DriftLeft(float AxisValue);
	void Accelerate();
	void Deccelerate();
	void Brake();
	void LiftBrake();
	void Raycast();
};
