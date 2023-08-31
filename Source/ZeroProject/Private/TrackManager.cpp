#include "TrackManager.h"
#include "AdaptativeMachine.h"
#include "PlayerMachine.h"

UTrackManager::UTrackManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


void UTrackManager::BeginPlay()
{
	Super::BeginPlay();

}


void UTrackManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateMachinesRank();
}

ASplineActor* UTrackManager::GetNextSpline(int splineIndex) {
	if (Splines.Num() > 0) {
		int nextIndex = splineIndex % Splines.Num();
		return Splines[nextIndex];
	}
	else {
		return 0;
	}
}

void UTrackManager::SetMachineProgress(int inID, float inProgress) {
	MachinesProgress[inID] = inProgress;
}

void UTrackManager::UpdateMachinesRank() {
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

void UTrackManager::ReportDisabledMachine() {
	ActiveMachines -= 1;
}