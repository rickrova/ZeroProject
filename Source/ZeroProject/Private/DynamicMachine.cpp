// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicMachine.h"

// Sets default values
ADynamicMachine::ADynamicMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));
}

void ADynamicMachine::BeginPlay()
{
	Super::BeginPlay();
	AlignToSurface(0);
	//TimerDelegate.BindUFunction(this, FName("SetDetour"));
	bDetourAvailable = true;
	//int rand = FMath::RandRange(0, 5);
	//GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 5, false, 5);
}

void ADynamicMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//ComputeDirection(DeltaTime);
	ComputeMovement(DeltaTime);
	AlignToSurface(DeltaTime);
	//CheckSplineProgress();
}

void ADynamicMachine::ComputeDirection(float deltaTime) {
	if (bDetourAvailable) {
		bDetourAvailable = false;
		bOnDetour = true;
		DesiredDetourAngle = FMath::RandRange(-5, 5);
	}
	if (bOnDetour) {
		CurrentDetourAngle = FMath::Lerp(CurrentDetourAngle, DesiredDetourAngle, deltaTime * 0.25f);
		if (DesiredDetourAngle != 0 && FMath::Abs(CurrentDetourAngle - DesiredDetourAngle) < 1) {
			DesiredDetourAngle = 0;
		}
		else if (DesiredDetourAngle == 0 && FMath::Abs(CurrentDetourAngle - DesiredDetourAngle) < 1) {
			CurrentDetourAngle = 0;
			bOnDetour = false;
			bDetourAvailable = true;
		}
	}
}

void ADynamicMachine::ComputeMovement(float deltaTime) {
	/*if (Speed < MaxSpeed) {
		Speed += AccelerationRate * deltaTime;
		Speed = FMath::Clamp(Speed, 0, MaxSpeed);
	}
	if (!bGrounded) {
		SurfaceAtractionSpeed -= SurfaceAtractionForce * deltaTime;
	}
	if (BounceSpeed > 0) {
		BounceSpeed -= BounceDeccelerationRate * deltaTime;
		BounceSpeed = FMath::Clamp(BounceSpeed, 0, MaxSpeed * 10);
	}
	FVector desiredDeltaLocation = GetActorForwardVector() * Speed * deltaTime
		+ SurfaceNormal * SurfaceAtractionSpeed * deltaTime
		+ BounceDirection * BounceSpeed * deltaTime;
	FHitResult* hit = new FHitResult();

	RootComponent->MoveComponent(desiredDeltaLocation, RootComponent->GetComponentRotation(),
		true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
	if (hit->bBlockingHit) {
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("hit")));
	}*/

	if (!bGrounded) {
		SurfaceAtractionSpeed -= SurfaceAtractionForce * deltaTime;
	}
	VisibleComponent->AddForce(VisibleComponent->GetForwardVector() * 10000000, NAME_None, false);
}

void ADynamicMachine::AlignToSurface(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation() + GetActorUpVector() * TraceUpDistance;
	FVector traceEnd = traceStart - GetActorUpVector() * (TraceUpDistance + TraceDownDistance);

	//ClosestSplinePoint = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
		SurfaceNormal = hit->ImpactNormal;
		SurfacePoint = hit->ImpactPoint;
		FVector deltaLocation = SurfacePoint + GetActorUpVector() * DistanceToSurface - GetActorLocation();
		FVector desiredDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
			ESplineCoordinateSpace::World);
		FVector currentDirection = FMath::Lerp(GetActorForwardVector(),
			desiredDirection, Steering * deltaTime);
		RootComponent->MoveComponent(deltaLocation, FRotationMatrix::MakeFromXZ(currentDirection, SurfaceNormal).Rotator(),
			true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
		if (hit->bBlockingHit) {
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("hit")));
		}
		if (!bGrounded) {
			bGrounded = true;
			SurfaceAtractionSpeed = 0;
		}
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("grounded")));
	}
	else {
		DesiredDetourAngle = 0;
		CurrentDetourAngle = 0;

		FVector desiredForward = FMath::Lerp(GetActorForwardVector(),
			Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World), Steering * deltaTime);
		ComputeClosestSurfaceNormal(deltaTime);
		FVector desiredUp = FMath::Lerp(GetActorUpVector(), SurfaceNormal, Steering * deltaTime);
		SetActorRotation(FRotationMatrix::MakeFromZX(desiredUp, desiredForward).Rotator());
		if (bGrounded) {
			bGrounded = false;
		}
	}
}

void ADynamicMachine::ComputeClosestSurfaceNormal(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation();
	FVector traceEnd = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
		SurfaceNormal = hit->Normal;
		//SurfacePoint = hit->ImpactPoint;
	}
	else {
		//SurfaceNormal = (traceStart - traceEnd).GetSafeNormal();
		//SurfacePoint = FVector::ZeroVector;
	}
}

void ADynamicMachine::CheckSplineProgress() {

	float progress = Spline->Spline->FindInputKeyClosestToWorldLocation(GetActorLocation())
		/ (Spline->Spline->GetNumberOfSplinePoints() - 1.f);
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("progress: %f"), progress));
	if (progress == 1) {
		CompletedSegments += 1;
		Spline = TrackManager->GetNextSpline(CompletedSegments);
	}
}

