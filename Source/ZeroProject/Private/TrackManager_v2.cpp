// Fill out your copyright notice in the Description page of Project Settings.


#include "TrackManager_v2.h"
#include "AdaptativeMachine.h"
#include "PlayerMachine.h"
#include "ZeroHUD.h"

// Sets default values
ATrackManager_v2::ATrackManager_v2()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Laps = 3;
	PlayerCurrentLap = 1;
	bCanCheckLap = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Goal = CreateDefaultSubobject<UBoxComponent>(TEXT("Goal"));
	CheckPoint = CreateDefaultSubobject<UBoxComponent>(TEXT("CheckPoint"));
	EndLapVerification = CreateDefaultSubobject<UBoxComponent>(TEXT("EndLapVerification"));

	SetRootComponent(Root);
	Goal->SetupAttachment(Root);
	CheckPoint->SetupAttachment(Root);
	EndLapVerification->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void ATrackManager_v2::BeginPlay()
{
	Super::BeginPlay();

	Goal->OnComponentBeginOverlap.AddDynamic(this, &ATrackManager_v2::OnGoalBeginOverlap);
	CheckPoint->OnComponentBeginOverlap.AddDynamic(this, &ATrackManager_v2::OnCheckPointBeginOverlap);
	EndLapVerification->OnComponentBeginOverlap.AddDynamic(this, &ATrackManager_v2::OnEndBeginOverlap);

	for (int i = 0; i < AIMachines.Num(); ++i) {
		AIMachines[i]->SetupTrackManager(this, i);
		MachinesProgress.Add(0.f);
		MachinesLaps.Add(0);
	}
	for (int i = 0; i < PlayersMachines.Num(); ++i) {
		PlayersMachines[i]->SetupTrackManager(this, AIMachines.Num() + i);
		MachinesProgress.Add(0.f);
		MachinesLaps.Add(0);
	}
	ActiveMachines = AIMachines.Num() + PlayersMachines.Num();
}

// Called every frame
void ATrackManager_v2::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateMachinesRank();
}

ASplineActor* ATrackManager_v2::GetNextSpline(int splineIndex) {
	if (Splines.Num() > 0) {
		int nextIndex = splineIndex % Splines.Num();
		return Splines[nextIndex];
	}
	else {
		return 0;
	}
}

void ATrackManager_v2::SetMachineProgress(int inID, float inProgress) {
	MachinesProgress[inID] = inProgress;
}

void ATrackManager_v2::UpdateMachinesRank() {
	TArray<float> orderedProgress = MachinesProgress;
	orderedProgress.Sort();
	for (int i = 0; i < MachinesProgress.Num(); ++i) {
		int rank = MachinesProgress.Num() - orderedProgress.IndexOfByKey(MachinesProgress[i]);
		if (i < AIMachines.Num()) {
			AIMachines[i]->SetRank(rank);
		}
		else {
			PlayersMachines[i - AIMachines.Num()]->SetRank(rank);
		}
	}
}

void ATrackManager_v2::ReportDisabledMachine() {
	ActiveMachines -= 1;
}

void ATrackManager_v2::Pause() {
	bool isPaused = !GetWorld()->GetFirstPlayerController()->IsPaused();
	GetWorld()->GetFirstPlayerController()->SetPause(isPaused);
	if (isPaused) {
		HUD->ShowPauseMenu();
	}
	else {
		HUD->HidePauseMenu();
	}
}

void ATrackManager_v2::RestartRace() {
	GetWorld()->GetFirstPlayerController()->RestartLevel();
}

void ATrackManager_v2::OnGoalBeginOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (OtherActor == PlayersMachines[0]) {
		bCheckPointReached = false;
		if (bCanCheckLap) {
			UE_LOG(LogTemp, Warning, TEXT("Player Lap"));
			PlayerCurrentLap += 1;
			bCanCheckLap = false;
			if (PlayerCurrentLap > Laps) {
				MachinesNamesRanked.Add(FText::FromString(OtherActor->GetName()));
				PlayersMachines[0]->Disable();
				HUD->ShowEndRaceScreen();
			}
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("AI Lap"));
		int currentIndex = AIMachines. IndexOfByKey(OtherActor);
		int currentLap = MachinesLaps[currentIndex] + 1;
		MachinesLaps[currentIndex] = currentLap;
		if (currentLap > Laps) {
			MachinesNamesRanked.Add(FText::FromString(OtherActor->GetName()));
			AIMachines[currentIndex]->Disable();
		}
	}
}

void ATrackManager_v2::OnCheckPointBeginOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (OtherActor == PlayersMachines[0]) {
		UE_LOG(LogTemp, Warning, TEXT("Checkpoint"));
		bCheckPointReached = true;
	}
}

void ATrackManager_v2::OnEndBeginOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (OtherActor == PlayersMachines[0]) {
		if (bCheckPointReached) {
			UE_LOG(LogTemp, Warning, TEXT("End Lap"));
			bCanCheckLap = true;
		}
	}
}

