// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineActor.h"
#include "Components/BoxComponent.h"
#include "TrackManager_v2.generated.h"

class AAdaptativeMachine;	// forward declaration
class APlayerMachine;	// forward declaration
class AZeroHUD;

UCLASS()
class ZEROPROJECT_API ATrackManager_v2 : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATrackManager_v2();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
protected:
	TArray<float> MachinesProgress;
	bool bCanCheckLap;
	bool bCheckPointReached;

	UPROPERTY(VisibleAnywhere)
		TArray<int> MachinesLaps;
	UPROPERTY(VisibleAnywhere)
		TArray<FText> MachinesNamesRanked;

public:

	int PlayerCurrentLap;
	int ActiveMachines;
	AZeroHUD* HUD;

	UPROPERTY(EditAnywhere)
		int Laps;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<ASplineActor*> Splines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<AAdaptativeMachine*> AIMachines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<APlayerMachine*> PlayersMachines;
	UPROPERTY(VisibleAnywhere)
		USceneComponent* Root;
	UPROPERTY(VisibleAnywhere)
		UBoxComponent* Goal;
	UPROPERTY(VisibleAnywhere)
		UBoxComponent* CheckPoint;
	UPROPERTY(VisibleAnywhere)
		UBoxComponent* EndLapVerification;

	ASplineActor* GetNextSpline(int splineIndex);
	void SetMachineProgress(int inID, float inProgress);
	void UpdateMachinesRank();
	void ReportDisabledMachine();

	UFUNCTION(BlueprintCallable)
		void Pause();
	UFUNCTION(BlueprintCallable)
		void RestartRace();

	UFUNCTION(BlueprintImplementableEvent, Category = "CodeEvents")
		void StartRace();

	UFUNCTION()
		void OnGoalBeginOverlap(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnEndBeginOverlap(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnCheckPointBeginOverlap(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
