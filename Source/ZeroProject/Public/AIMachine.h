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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* VisibleComponent;
    UPROPERTY(VisibleAnywhere)
        USkeletalMeshComponent* Guide;
	UPROPERTY(EditAnywhere)
		float MaxSpeed = 1.0f;
	UPROPERTY(EditAnywhere)
		float AccelerationRate = 100.0f;
	UPROPERTY(BlueprintReadOnly)
		float Speed = 0.f;

};