#include "AdaptativeMachine.h"

AAdaptativeMachine::AAdaptativeMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));
}

void AAdaptativeMachine::BeginPlay()
{
	Super::BeginPlay();
	AlignToSurface(0);
	//TimerDelegate.BindUFunction(this, FName("SetDetour"));
	bDetourAvailable = true;
	//int rand = FMath::RandRange(0, 5);
	//GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 5, false, 5);
}

void AAdaptativeMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AAdaptativeMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ComputeDirection(DeltaTime);
	ComputeMovement(DeltaTime);
	CheckTrackProgress();
	AlignToSurface(DeltaTime);
	CheckAvoidables();
}

void AAdaptativeMachine::ComputeDirection(float deltaTime) {

	// TrackDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
	// 	ESplineCoordinateSpace::World);

	float rawAvoidDelta = NormalizedDesiredAvoidAmount - NormalizedCurrentAvoidAmount;
	float normalizedAvoidDelta = FMath::Min(deltaTime * Steering, FMath::Abs(rawAvoidDelta)) * FMath::Sign(rawAvoidDelta);

	NormalizedCurrentAvoidAmount = FMath::Clamp(NormalizedCurrentAvoidAmount + normalizedAvoidDelta, 0, 1);
	MachineDirection = FMath::Lerp(GetActorForwardVector(), AvoidDirection, NormalizedCurrentAvoidAmount);
	/*GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("AvoidDirection: %f, %f, %f"),
		AvoidDirection.X,
		AvoidDirection.Y,
		AvoidDirection.Z));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("NormalizedDesiredAvoidAmount: %f"), NormalizedDesiredAvoidAmount));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("NormalizedCurrentAvoidAmount: %f"), NormalizedCurrentAvoidAmount));
	*/



	//if (bDetourAvailable) {
	//	bDetourAvailable = false;
	//	bOnDetour = true;
	//	DesiredDetourAngle = FMath::RandRange(-5, 5);
	//}
	//if (bOnDetour) {
	//	CurrentDetourAngle = FMath::Lerp(CurrentDetourAngle, DesiredDetourAngle, deltaTime * 0.25f);
	//	if (DesiredDetourAngle != 0 && FMath::Abs(CurrentDetourAngle - DesiredDetourAngle) < 1) {
	//		DesiredDetourAngle = 0;
	//	}
	//	else if (DesiredDetourAngle == 0 && FMath::Abs(CurrentDetourAngle - DesiredDetourAngle) < 1) {
	//		CurrentDetourAngle = 0;
	//		bOnDetour = false;
	//		bDetourAvailable = true;
	//		//int rand = FMath::RandRange(0, 5);
	//		//GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 5, false, 5);
	//	}
	//}
}

void AAdaptativeMachine::ComputeMovement(float deltaTime) {
	if (Speed < MaxSpeed) {
		Speed += AccelerationRate * deltaTime;
	}
	else {
		Speed -= AccelerationRate * deltaTime;
	}
	if (!bGrounded) {
		SurfaceAtractionSpeed += SurfaceAtractionForce * deltaTime;
	}
	if (BounceSpeed > 0) {
		BounceSpeed -= BounceDeccelerationRate * deltaTime;
	}
	else {
		BounceSpeed += BounceDeccelerationRate * deltaTime;
	}
	FVector desiredDeltaLocation = MachineDirection * Speed * deltaTime
		- SurfaceNormal * SurfaceAtractionSpeed * deltaTime
		+ BounceDirection * BounceSpeed * deltaTime;
	LastDeltaLocation = desiredDeltaLocation / deltaTime;
	FHitResult* hit = new FHitResult();
	
	RootComponent->MoveComponent(desiredDeltaLocation, FRotationMatrix::MakeFromXZ(MachineDirection, SurfaceNormal).Rotator(),
		true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
	if (hit->bBlockingHit) {
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("hit")));
		Bounce(hit, LastDeltaLocation);
	}

	/*RealSpeed = FVector::Distance(GetActorLocation(), LastActorLocation);
	LastActorLocation = GetActorLocation();
	if (RealSpeed < desiredDeltaLocation.Length()) {
		float ratio = RealSpeed / desiredDeltaLocation.Length();
		Speed *= ratio;
		SurfaceAtractionSpeed *= ratio;
		BounceSpeed *= ratio;
	}*/
}

void AAdaptativeMachine::AlignToSurface(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation() + GetActorUpVector() * TraceUpDistance;
	FVector traceEnd = traceStart - GetActorUpVector() * (TraceUpDistance + TraceDownDistance);
	TrackDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
		ESplineCoordinateSpace::World);

	//ClosestSplinePoint = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
		SurfacePoint = hit->ImpactPoint;
		SurfaceNormal = hit->ImpactNormal;
		FVector deltaLocation = SurfacePoint + GetActorUpVector() * DistanceToSurface - GetActorLocation();
		/*FVector desiredDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
			ESplineCoordinateSpace::World);
		if (bOnDetour) {
			desiredDirection = desiredDirection.RotateAngleAxis(CurrentDetourAngle, GetActorUpVector());
		}
		FVector currentDirection = FMath::Lerp(GetActorForwardVector(),
			desiredDirection, Steering * deltaTime);*/
		RootComponent->MoveComponent(deltaLocation, FRotationMatrix::MakeFromZX(SurfaceNormal, TrackDirection).Rotator(),
			true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
			//LastMachineDirection = GetActorUpVector();
		if (hit->bBlockingHit) {
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("hit")));
			Bounce(hit, deltaLocation);
		}
		if (!bGrounded) {
			bGrounded = true;
			SurfaceAtractionSpeed = 0;
		}
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("grounded")));
	}
	else {
		ComputeClosestSurfaceNormal(deltaTime);
		SetActorRotation(FRotationMatrix::MakeFromZX(SurfaceNormal, TrackDirection).Rotator());
		if (bGrounded) {
			bGrounded = false;

			Push(LastDeltaLocation);

			if (BounceSpeed > Speed / 4) {
				//Destroy();
			}

			/*
			//NormalizedDesiredAvoidAmount = 0.f;
			float dot = FVector::DotProduct(LastMachineDirection, -TrackDirection);
			float angleRatio = FMath::Asin(dot) / PI * 2;
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("PR: %f"), angleRatio));

			SurfaceAtractionSpeed = -Speed * angleRatio;
			Speed += SurfaceAtractionSpeed;
			*/
		}
	}
}

void AAdaptativeMachine::ComputeClosestSurfaceNormal(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation();
	FVector traceEnd = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);
	

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
		SurfaceNormal = (GetActorLocation() - hit->ImpactPoint).GetSafeNormal(); 
	}
	else {
		traceStart = traceEnd;
		FVector dir = (traceStart - GetActorLocation()).GetSafeNormal();
		traceEnd = traceStart + dir * 10000;

		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
			SurfaceNormal = dir;
		}
	}
}

void AAdaptativeMachine::Bounce(FHitResult* hit, FVector deltaLocation) {
	AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
	
	if (otherMachine) {
		FVector residualVerticalBounce = otherMachine->Push(LastDeltaLocation);
		if (bGrounded) {
			residualVerticalBounce = FVector::ZeroVector;
		}
		Push(otherMachine->LastDeltaLocation - residualVerticalBounce);
	}
	else {
		FVector reflectedVelocty = LastDeltaLocation.MirrorByVector(hit->Normal);
		Push(reflectedVelocty);
	}

}

void AAdaptativeMachine::CheckTrackProgress() {

	float progress = Spline->Spline->FindInputKeyClosestToWorldLocation(GetActorLocation())
		/ (Spline->Spline->GetNumberOfSplinePoints() - 1.f);
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("progress: %f"), progress));
	if (progress == 1) {
		CurrentSegment += 1;
		Spline = TrackManager->GetNextSpline(CurrentSegment);
	}
}

void AAdaptativeMachine::CheckAvoidables() {

	FHitResult* hit = new FHitResult();
	float rightCollisionDistance = 0;
	float leftCollisionDistance = 0;
	bool bPrioritizeAvoidFalling = false;
	FVector traceStart;
	FVector traceEnd;

	//Check for risk of falling
	if (bGrounded) {
		//Down right
		traceStart = GetActorLocation() + GetActorRightVector() * 300 + GetActorUpVector() * 200 + GetActorForwardVector() * 200;
		traceEnd = traceStart - GetActorUpVector() * 1200.f;
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else if (NormalizedCurrentAvoidAmount < 0.5f) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = 0.05f;
			//NormalizedCurrentAvoidAmount = NormalizedDesiredAvoidAmount;
			AvoidDirection = -GetActorRightVector();
			bPrioritizeAvoidFalling = true;
		}
		//Down left
		traceStart = GetActorLocation() - GetActorRightVector() * 300 + GetActorUpVector() * 200 + GetActorForwardVector() * 200;
		traceEnd = traceStart - GetActorUpVector() * 1200.f;
		DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else if (NormalizedCurrentAvoidAmount < 0.5f) {

			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = 0.05f;
			//NormalizedCurrentAvoidAmount = NormalizedDesiredAvoidAmount;
			AvoidDirection = GetActorRightVector();
			bPrioritizeAvoidFalling = true;
		}
	}

		//Check for collision
		if (!bPrioritizeAvoidFalling) {
			FCollisionQueryParams queryParams;
			queryParams.AddIgnoredActor(this);
			//FVector flatDirection = FVector::VectorPlaneProject(MachineDirection, GetActorUpVector()).GetSafeNormal();
			FVector flatRight = FVector::CrossProduct(MachineDirection, GetActorUpVector());

			//Front right
			traceStart = GetActorLocation() - MachineDirection * 200 + flatRight * 300; // *(Speed / MaxSpeed);
			traceEnd = GetActorLocation() + MachineDirection * 8000.f; // *(Speed / MaxSpeed);
			float traceDistance = FVector::Distance(traceStart, traceEnd);
			if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_Vehicle, queryParams)) {
				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
				rightCollisionDistance = (traceDistance - hit->Distance) / traceDistance;
				//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("right distance: %f"), rightCollisionDistance));
			}
			else {

				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
			}
			//Front left
			traceStart = GetActorLocation() - MachineDirection * 200 - flatRight * 300; // *(Speed / MaxSpeed);
			traceEnd = GetActorLocation() + MachineDirection * 8000.f; // *(Speed / MaxSpeed);
			if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_Vehicle, queryParams)) {
				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
				leftCollisionDistance = (traceDistance - hit->Distance) / traceDistance;
				//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("left distance: %f"), leftCollisionDistance));
			}
			else {
				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
			}

			float delta = rightCollisionDistance - leftCollisionDistance;
			float maxDistance = FMath::Max(rightCollisionDistance, leftCollisionDistance);
			float speedDecimation = MaxSpeed * (1 - maxDistance);  // *0.5f;
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("delta ijo: %f"), delta));
			if (FMath::Abs(delta) > 0.0f) {

				NormalizedDesiredAvoidAmount = maxDistance;
				AvoidDirection = GetActorRightVector() * FMath::Sign(delta);
			if (FMath::Abs(delta) < 0.05f && Speed > speedDecimation)
			{
				//Speed = speedDecimation;
			}
			}
			else {
				NormalizedDesiredAvoidAmount = 0;
			}
		}
	//}
}

void AAdaptativeMachine::SetDetour() {
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Airpitch: %s"), *GetActorLabel()));
	bDetourAvailable = true;
}

FVector AAdaptativeMachine::Push(FVector pushVelocity) {
	FVector pushDirection = pushVelocity.GetSafeNormal();
	float pushMagnitude = pushVelocity.Size();

	float forwardVelocityProjection = FVector::DotProduct(GetActorForwardVector(), pushDirection);
	Speed = forwardVelocityProjection * pushMagnitude;

	float verticalVelocityProjection = FVector::DotProduct(GetActorUpVector(), pushDirection);
	float verticalSpeed = verticalVelocityProjection * pushMagnitude;

	FVector rawBounce = pushVelocity - GetActorForwardVector() * Speed - GetActorUpVector() * verticalSpeed;
	BounceDirection = rawBounce.GetSafeNormal();
	BounceSpeed = rawBounce.Size();

	if (bGrounded) {
		SurfaceAtractionSpeed = 0;
		//return (GetActorUpVector() + GetActorForwardVector()) * -4000;
		return GetActorUpVector() * verticalSpeed;
		//Speed += verticalSpeed;
		//return FVector::Zero();
	}
	else {
		SurfaceAtractionSpeed = -verticalSpeed;
		return FVector::Zero();
	}
}
