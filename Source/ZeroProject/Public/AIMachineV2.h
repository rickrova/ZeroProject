// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIMachineV2.generated.h"

UCLASS()
class ZEROPROJECT_API AAIMachineV2 : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AAIMachineV2();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
