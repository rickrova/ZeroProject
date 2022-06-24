#include "TrackManager.h"

UTrackManager::UTrackManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

}


void UTrackManager::BeginPlay()
{
	Super::BeginPlay();
	
}


void UTrackManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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

