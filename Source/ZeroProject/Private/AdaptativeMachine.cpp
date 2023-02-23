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
}

void AAdaptativeMachine::ComputeDirection(float deltaTime) {

	CheckAvoidables();
	FVector trackDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
		ESplineCoordinateSpace::World);

	float rawAvoidDelta = NormalizedDesiredAvoidAmount - NormalizedCurrentAvoidAmount;
	float normalizedAvoidDelta = FMath::Min(deltaTime * Steering, FMath::Abs(rawAvoidDelta)) * FMath::Sign(rawAvoidDelta);
	NormalizedCurrentAvoidAmount = FMath::Clamp(NormalizedCurrentAvoidAmount +  normalizedAvoidDelta, 0, 1);
	MachineDirection = FMath::Lerp(trackDirection, AvoidDirection, NormalizedCurrentAvoidAmount);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("AvoidDirection: %f, %f, %f"),
		AvoidDirection.X,
		AvoidDirection.Y,
		AvoidDirection.Z));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("NormalizedDesiredAvoidAmount: %f"), NormalizedDesiredAvoidAmount));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("NormalizedCurrentAvoidAmount: %f"), NormalizedCurrentAvoidAmount));



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
		Speed = FMath::Clamp(Speed, 0, MaxSpeed);
	}
	if (!bGrounded) {
		SurfaceAtractionSpeed += SurfaceAtractionForce * deltaTime;
	}
	if (BounceSpeed > 0) {
		BounceSpeed -= BounceDeccelerationRate * deltaTime;
		BounceSpeed = FMath::Clamp(BounceSpeed, 0, MaxSpeed * 10);
	}
	FVector desiredDeltaLocation = MachineDirection * Speed * deltaTime
		- SurfaceNormal * SurfaceAtractionSpeed * deltaTime
		+ BounceDirection * BounceSpeed * deltaTime;
	LastDeltaLocation = MachineDirection * Speed
		- SurfaceNormal * SurfaceAtractionSpeed
		+ BounceDirection * BounceSpeed;
	FHitResult* hit = new FHitResult();
	
	RootComponent->MoveComponent(desiredDeltaLocation, FRotationMatrix::MakeFromXZ(MachineDirection, SurfaceNormal).Rotator(),
		true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
	if (hit->bBlockingHit) {
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("hit")));
		Bounce(hit);
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
		RootComponent->MoveComponent(deltaLocation, FRotationMatrix::MakeFromZX(SurfaceNormal, GetActorForwardVector()).Rotator(),
			true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
		if (hit->bBlockingHit) {
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("hit")));
			Bounce(hit);
		}
		if (!bGrounded) {
			bGrounded = true;
			SurfaceAtractionSpeed = 0;
		}
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("grounded")));
	}
	else {
		ComputeClosestSurfaceNormal(deltaTime);
		SetActorRotation(FRotationMatrix::MakeFromZX(SurfaceNormal, GetActorForwardVector()).Rotator());
		if (bGrounded) {
			bGrounded = false;
		}
	}
}

void AAdaptativeMachine::ComputeClosestSurfaceNormal(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation();
	FVector traceEnd = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
		SurfaceNormal = (GetActorLocation() - hit->ImpactPoint).GetSafeNormal(); //hit->Normal;
		//SurfacePoint = hit->ImpactPoint;
	}
	else {
		//SurfaceNormal = (traceStart - traceEnd).GetSafeNormal();
		//SurfacePoint = FVector::ZeroVector;
	}
}

void AAdaptativeMachine::Bounce(FHitResult* hit) {
	AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
	//float speedDifference = FMath::Abs(otherMachine->Speed - Speed) / MaxSpeed;
	
	/*FRotator deltaRotator = FRotator::ZeroRotator;
	float side = FVector::DotProduct(BounceDirection, GetActorRightVector());
	if (FMath::Abs(side) < 0.25f && Speed > MaxSpeed * 0.25f) {
		side = FMath::Sign(FMath::RandRange(0, 1) - 0.5f) * 2;
	}
	deltaRotator.Yaw = SideBounceDeviationAngle * side;
	RootComponent->AddLocalRotation(deltaRotator);*/

	//DesiredDetourAngle *= -1;
	//CurrentDetourAngle *= -1;

	if (otherMachine) {

		float speedDecimation = FVector::DotProduct(GetActorForwardVector(), -hit->Normal);
		//BounceSpeed = Speed * speedDecimation;
		Speed *= 1 - speedDecimation;
		//SurfaceAtractionSpeed = 0;

		BounceDirection = hit->Normal; //FVector(vX1, vY1, vZ1).GetSafeNormal(); //hit->Normal;
		float bounceDelta = 2000; // otherMachine->LastDeltaLocation.ProjectOnToNormal(BounceDirection).Size();
		BounceSpeed = bounceDelta;
		FVector desiredForward = (GetActorForwardVector() + BounceDirection).GetSafeNormal();
		SetActorRotation(FRotationMatrix::MakeFromZX(GetActorUpVector(), desiredForward).Rotator());
		//GEngine->AddOnScreenDebugMessage(-1, 100.f, FColor::Yellow, FString::Printf(TEXT("bounce speed = %f"), BounceSpeed));

		FVector otherDirection = -BounceDirection; // FVector(vX2, vY2, vZ2).GetSafeNormal(); //-BounceDirection;
		float otherSpeed = BounceSpeed; // LastDeltaLocation.ProjectOnToNormal(otherDirection).Size();

		otherMachine->Push(otherDirection, otherSpeed);
		//GEngine->AddOnScreenDebugMessage(-1, 100.f, FColor::Yellow, FString::Printf(TEXT("original machine: %s"), *GetActorNameOrLabel()));
		
	}
	else {

		BounceDirection = hit->Normal;
		float speedDecimation = FVector::DotProduct(GetActorForwardVector(), -BounceDirection);
		BounceSpeed = FMath::Max(2000.f, Speed* speedDecimation);
		Speed *= 1 - speedDecimation;
		SurfaceAtractionSpeed = 0; //this is just in case collision occurs against a wall
	}

}

void AAdaptativeMachine::CheckTrackProgress() {

	float progress = Spline->Spline->FindInputKeyClosestToWorldLocation(GetActorLocation())
		/ (Spline->Spline->GetNumberOfSplinePoints() - 1.f);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("progress: %f"), progress));
	if (progress == 1) {
		CompletedSegments += 1;
		Spline = TrackManager->GetNextSpline(CompletedSegments);
	}
}

void AAdaptativeMachine::CheckAvoidables() {

	FHitResult* hit = new FHitResult();
	float rightCollisionDistance = 0;
	float leftCollisionDistance = 0;
	bool bPrioritizeAvoidFalling;
	FVector traceStart;
	FVector traceEnd;

	//Check for risk of falling
	if (bGrounded) {
		//Down right
		traceStart = GetActorLocation() + GetActorRightVector() * 300;
		traceEnd = traceStart - GetActorUpVector() * 1000.f;
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else if(NormalizedCurrentAvoidAmount < 0.05f){
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = 0.5f;
			AvoidDirection = -GetActorRightVector();
			bPrioritizeAvoidFalling = true;
		}
		//Down left
		traceStart = GetActorLocation() - GetActorRightVector() * 300;
		traceEnd = traceStart - GetActorUpVector() * 1000.f;
		DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel1)) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else if (NormalizedCurrentAvoidAmount < 0.05f) {

			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = 0.5f;
			AvoidDirection = GetActorRightVector();
			bPrioritizeAvoidFalling = true;
		}

		//Check for collision
		if (!bPrioritizeAvoidFalling) {
			//Front right
			traceStart = GetActorLocation() - GetActorForwardVector() * 200 + GetActorRightVector() * 300;
			traceEnd = GetActorLocation() + GetActorForwardVector() * 4000.f;
			float traceDistance = FVector::Distance(traceStart, traceEnd);
			if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_Vehicle)) {
				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
				rightCollisionDistance = (traceDistance - hit->Distance) / traceDistance;
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("right distance: %f"), rightCollisionDistance));
			}
			else {

				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
			}
			//Front left
			traceStart = GetActorLocation() - GetActorForwardVector() * 200 - GetActorRightVector() * 300;
			traceEnd = GetActorLocation() + GetActorForwardVector() * 4000.f;
			if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_Vehicle)) {
				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
				leftCollisionDistance = (traceDistance - hit->Distance) / traceDistance;
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("left distance: %f"), leftCollisionDistance));
			}
			else {
				DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
			}
			if (rightCollisionDistance != 0 || leftCollisionDistance != 0) {
				int side = FMath::Sign(leftCollisionDistance - rightCollisionDistance);
				NormalizedDesiredAvoidAmount = FMath::Max(NormalizedDesiredAvoidAmount, FMath::Max(rightCollisionDistance, leftCollisionDistance));
				AvoidDirection = GetActorRightVector() * side;
			}
			else {
				NormalizedDesiredAvoidAmount = 0;
			}
		}
	}
}

void AAdaptativeMachine::SetDetour() {
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Airpitch: %s"), *GetActorLabel()));
	bDetourAvailable = true;
}

void AAdaptativeMachine::Push(FVector bounceDirection, float bounceSpeed) {
	BounceDirection = bounceDirection;
	BounceSpeed = bounceSpeed;
	float speedDecimation = FMath::Clamp(FVector::DotProduct(GetActorForwardVector(), -BounceDirection), 0.25f, 1.f);
	Speed *= 1 - speedDecimation;

	FVector desiredForward = (GetActorForwardVector() + BounceDirection).GetSafeNormal();
	SetActorRotation(FRotationMatrix::MakeFromZX(GetActorUpVector(), desiredForward).Rotator());
	//GEngine->AddOnScreenDebugMessage(-1, 100.f, FColor::Yellow, FString::Printf(TEXT("pushed machine: %s"), *GetActorNameOrLabel()));
}
