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
	// Sets default values for this actor's properties
	AAIMachine();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
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
		float MaxSpeed = 1.0f;
    UPROPERTY(EditAnywhere)
        float AccelerationRate = 100.0f;
    UPROPERTY(EditAnywhere)
        float Steering = 10.f;
    UPROPERTY(EditAnywhere)
        float DeltaX = 0.f;
    UPROPERTY(EditAnywhere)
        float DistanceToFloor = 20.f;
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
    float VerticalSpeed = 0;
    float LastVerticalDelta;
    float LastHeight;
	bool bCanRace = false;
	bool bCanSetNewDesiredDeltaX = false;
    bool bGrounded = true;
    FVector LastDirection;
	FVector LastSurfaceLocation;
	FVector DesiredLocation;
	FRotator DesiredRotation;
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;

	UFUNCTION()
		void StartRace();
    
    void SetHeight(float deltaTime);

public:
	UPROPERTY(BlueprintReadWrite)
		float PreSpeed = 0.f;
    
    float DesiredDeltaX = 0;
    
    void HitByMachine(float rightDot);
	void HitByMachine2(float forwardDot);
};
