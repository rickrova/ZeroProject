// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SplineActor.h"
#include "TrackManager.generated.h"

class AAdaptativeMachine;	// forward declaration
class APlayerMachine;	// forward declaration

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

protected:
	TArray<float> MachinesProgress;

public:

	UPROPERTY(VisibleAnywhere)
	int ActiveMachines;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
		//TSubclassOf<AAdaptativeMachine>  AIMachineClassReference;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
		//TSubclassOf<APlayerMachine>  PlayerMachineClassReference;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<ASplineActor*> Splines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<AAdaptativeMachine*> AIMachines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<APlayerMachine*> PlayersMachines;

	ASplineActor* GetNextSpline(int splineIndex);
	void SetMachineProgress(int inID, float inProgress);
	void UpdateMachinesRank();
	void ReportDisabledMachine();
};
