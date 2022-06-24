// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SplineActor.h"
#include "TrackManager.generated.h"


UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ZEROPROJECT_API UTrackManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTrackManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<ASplineActor*> Splines;

	ASplineActor* GetNextSpline(int splineIndex);

		
};
