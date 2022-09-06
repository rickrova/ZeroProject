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
	AlignToSurface(DeltaTime);
	CheckSplineProgress();
}

void AAdaptativeMachine::ComputeDirection(float deltaTime) {
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
			//int rand = FMath::RandRange(0, 5);
			//GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 5, false, 5);
		}
	}
}

void AAdaptativeMachine::ComputeMovement(float deltaTime) {
	if (Speed < MaxSpeed) {
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
	LastDeltaLocation = GetActorForwardVector() * Speed
		+ SurfaceNormal * SurfaceAtractionSpeed
		+ BounceDirection * BounceSpeed;
	FHitResult* hit = new FHitResult();
	
	RootComponent->MoveComponent(desiredDeltaLocation, RootComponent->GetComponentRotation(),
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

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
		SurfaceNormal = hit->ImpactNormal;
		SurfacePoint = hit->ImpactPoint;
		FVector deltaLocation = SurfacePoint + GetActorUpVector() * DistanceToSurface - GetActorLocation();
		FVector desiredDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
			ESplineCoordinateSpace::World);
		if (bOnDetour) {
			desiredDirection = desiredDirection.RotateAngleAxis(CurrentDetourAngle, GetActorUpVector());
		}
		FVector currentDirection = FMath::Lerp(GetActorForwardVector(),
			desiredDirection, Steering * deltaTime);
		RootComponent->MoveComponent(deltaLocation, FRotationMatrix::MakeFromXZ(currentDirection, SurfaceNormal).Rotator(),
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

void AAdaptativeMachine::ComputeClosestSurfaceNormal(float deltaTime) {
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

		float a, b, eX, eY, eZ;
		float c, d, fX, fY, fZ;

		a = Mass;
		b = otherMachine->Mass;
		eX = Mass * LastDeltaLocation.X + otherMachine->Mass * otherMachine->LastDeltaLocation.X;
		eY = Mass * LastDeltaLocation.Y + otherMachine->Mass * otherMachine->LastDeltaLocation.Y;
		eZ = Mass * LastDeltaLocation.Z + otherMachine->Mass * otherMachine->LastDeltaLocation.Z;

		c = -1;
		d = 1;
		fX = LastDeltaLocation.X - otherMachine->LastDeltaLocation.X;
		fY = LastDeltaLocation.Y - otherMachine->LastDeltaLocation.Y;
		fZ = LastDeltaLocation.Z - otherMachine->LastDeltaLocation.Z;

		float determinant = a * d - b * c;

		if (determinant != 0) {
			float vX1 = (eX * d - b * fX) / determinant;
			float vX2 = (a * fX - eX * c) / determinant;

			float vY1 = (eY * d - b * fY) / determinant;
			float vY2 = (a * fY - eY * c) / determinant;

			float vZ1 = (eZ * d - b * fZ) / determinant;
			float vZ2 = (a * fZ - eZ * c) / determinant;

			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Cramer equations system: result, vz1 = %f, vz2 = %f"), vZ1, vZ2));
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Cramer equations system: result, vy1 = %f, vy2 = %f"), vY1, vY2));
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Cramer equations system: result, vx1 = %f, vx2 = %f"), vX1, vX2));

			float speedDecimation = FVector::DotProduct(GetActorForwardVector(), -hit->Normal);
			//BounceSpeed = Speed * speedDecimation;
			Speed *= 1 - speedDecimation;
			//SurfaceAtractionSpeed = 0;

			BounceDirection = FVector(vX1, vY1, vZ1).GetSafeNormal(); //hit->Normal;
			float bounceDelta = otherMachine->LastDeltaLocation.ProjectOnToNormal(BounceDirection).Size();
			BounceSpeed = bounceDelta;
			FVector desiredForward = (GetActorForwardVector() + BounceDirection * (bounceDelta / Speed)).GetSafeNormal();
			SetActorRotation(FRotationMatrix::MakeFromZX(GetActorUpVector(), desiredForward).Rotator());
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("bounce speed = %f"), BounceSpeed));

			FVector otherDirection = FVector(vX2, vY2, vZ2).GetSafeNormal(); //-BounceDirection;
			float otherSpeed = LastDeltaLocation.ProjectOnToNormal(otherDirection).Size();

			otherMachine->Push(otherDirection, otherSpeed);
		}
		else {
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Cramer equations system: determinant is zero\n"
				"there are either no solutions or many solutions exist.")));
		}
	}
	else {

		BounceDirection = hit->Normal;
		float speedDecimation = FVector::DotProduct(GetActorForwardVector(), -BounceDirection);
		BounceSpeed = Speed * speedDecimation;
		Speed *= 1 - speedDecimation;
		SurfaceAtractionSpeed = 0; //this is just in case the collision occurs against a wall
	}

}

void AAdaptativeMachine::CheckSplineProgress() {

	float progress = Spline->Spline->FindInputKeyClosestToWorldLocation(GetActorLocation())
		/ (Spline->Spline->GetNumberOfSplinePoints() - 1.f);
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("progress: %f"), progress));
	if (progress == 1) {
		CompletedSegments += 1;
		Spline = TrackManager->GetNextSpline(CompletedSegments);
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

	FVector desiredForward = (GetActorForwardVector() + BounceDirection * (bounceSpeed / Speed)).GetSafeNormal();
	SetActorRotation(FRotationMatrix::MakeFromZX(GetActorUpVector(), desiredForward).Rotator());
}
