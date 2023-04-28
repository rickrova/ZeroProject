#include "AdaptativeMachine.h"

AAdaptativeMachine::AAdaptativeMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionComponent"));
	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));

	VisibleComponent->SetupAttachment(CollisionComponent);
}

void AAdaptativeMachine::BeginPlay()
{
	Super::BeginPlay();

	AlignToSurface(0);
	//TimerDelegate.BindUFunction(this, FName("SetDetour"));
	bDetourAvailable = true;
	bCanFindSurface = true;
	bStucked = false;
	//int rand = FMath::RandRange(0, 5);
	//GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 5, false, 5);

	VisibleRotationSpeed = 10;
	VisibleRotation = GetActorRotation();

	Condition = 1.f;
	CurrentAlterPathTimeThreshold = FMath::RandRange(0.25f, 3.f);
	CurrentBoostTimeThreshold = 2.5f;
	//VisibleComponent->SetupAttachment(CollisionComponent);
}

void AAdaptativeMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AAdaptativeMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!bDepleted){
		FVector initialLocation = GetActorLocation();
		ComputeDirection(DeltaTime);
		ComputeMovement(DeltaTime);
		CheckTrackProgress();
		AlignToSurface(DeltaTime);
		CheckAvoidables(DeltaTime);
		CheckForAlterPath();
		CheckForBoost();
		CheckStuck(DeltaTime, initialLocation);
		UpdateVisibleRotation(DeltaTime);
	}
	
}

void AAdaptativeMachine::ComputeDirection(float deltaTime) {

	// TrackDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
	// 	ESplineCoordinateSpace::World);

	float rawAvoidDelta = NormalizedDesiredAvoidAmount - NormalizedCurrentAvoidAmount;
	float normalizedAvoidDelta = FMath::Min(deltaTime * Steering, FMath::Abs(rawAvoidDelta)) * FMath::Sign(rawAvoidDelta);

	NormalizedCurrentAvoidAmount = FMath::Clamp(NormalizedCurrentAvoidAmount + normalizedAvoidDelta, 0, 1);
	MachineDirection = FMath::Lerp(GetActorForwardVector(), AvoidDirection, NormalizedCurrentAvoidAmount);
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("AvoidDirection: %f, %f, %f"),
		//AvoidDirection.X,
		//AvoidDirection.Y,
		//AvoidDirection.Z));
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("NormalizedDesiredAvoidAmount: %f"), NormalizedDesiredAvoidAmount));
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("NormalizedCurrentAvoidAmount: %f"), NormalizedCurrentAvoidAmount));
	



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
	if (BoostSpeed > 0) {
		BoostSpeed -= BoostDeccelerationRate * deltaTime;
		BoostSpeed = FMath::Clamp(BoostSpeed, 0, Boost);
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
	FVector desiredDeltaLocation = MachineDirection * (Speed + BoostSpeed) * deltaTime
		- SurfaceNormal * SurfaceAtractionSpeed * deltaTime
		+ BounceDirection * BounceSpeed * deltaTime;

	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("BounceDirection: %f, %f, %f, %s, %f"),
	//	BounceDirection.X,
	//	BounceDirection.Y,
	//	BounceDirection.Z,
	//	"BD:",
	//	BounceSpeed));

	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("SurfaceNormal: %f, %f, %f, %s, %f"),
	//	SurfaceNormal.X,
	//	SurfaceNormal.Y,
	//	SurfaceNormal.Z,
	//	"SAS:",
	//	SurfaceAtractionSpeed));

	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("MachineDirection: %f, %f, %f, %s, %f"),
	//	MachineDirection.X,
	//	MachineDirection.Y,
	//	MachineDirection.Z,
	//	"Speed:",
	//	Speed));


	//LastDeltaLocation = desiredDeltaLocation / deltaTime;
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

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
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
			//Bounce(hit, deltaLocation);
		}
		if (!bGrounded) {
			bGrounded = true;
			SurfaceAtractionSpeed = 0;
			VisibleRotation = GetActorRotation();
			VisibleRotationSpeed = 10;
		}
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("grounded")));
	}
	else if (bCanFindSurface) {
		ComputeClosestSurfaceNormal(deltaTime);
		SetActorRotation(FRotationMatrix::MakeFromZX(SurfaceNormal, TrackDirection).Rotator());
		if (bGrounded) {

			bGrounded = false;

			Push(LastDeltaLocation, false);
			VisibleRotationSpeed = 1.5f;
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("not grounded")));


			/*
			//NormalizedDesiredAvoidAmount = 0.f;
			float dot = FVector::DotProduct(LastMachineDirection, -TrackDirection);
			float angleRatio = FMath::Asin(dot) / PI * 2;
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("PR: %f"), angleRatio));

			SurfaceAtractionSpeed = -Speed * angleRatio;
			Speed += SurfaceAtractionSpeed;
			*/
		}
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() - SurfaceNormal * 500, FColor(255, 255, 0), false, 0.025f, 0, 10);
	}
	else if(FVector::Distance(GetActorLocation(),
		Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World)) > 10000){
		SoftDestroy();
	}
}

void AAdaptativeMachine::ComputeClosestSurfaceNormal(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation();
	FVector traceEnd = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);
	DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 255, 0), false, 0.025f, 0, 10);
	

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
		SurfaceNormal = hit->Normal; //(GetActorLocation() - hit->ImpactPoint).GetSafeNormal(); 
	}
	else {
		traceStart = traceEnd;
		FVector dir = (GetActorLocation() - traceStart).GetSafeNormal();
		traceEnd = traceStart + dir * 10000;
		DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 128, 0), false, 0.025f, 0, 10);

		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
			SurfaceNormal = hit->Normal;
		}
		else {
			//SoftDestroy();
			bCanFindSurface = false;
		}
	}
}

void AAdaptativeMachine::Bounce(FHitResult* hit, FVector deltaLocation) {
	AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
	
	if (otherMachine) {
		if (otherMachine->bStucked) {
			FVector reflectedVelocty = LastDeltaLocation.MirrorByVector(hit->Normal);
			Push(reflectedVelocty, true);
			otherMachine->SoftDestroy();
		}
		else {
			FVector residualVerticalBounce = otherMachine->Push(LastDeltaLocation, true);
			if (bGrounded) {
				residualVerticalBounce = FVector::ZeroVector;
			}
			Push(otherMachine->LastDeltaLocation + residualVerticalBounce, true);
		}
	}
	else {
		FVector reflectedVelocty = LastDeltaLocation.MirrorByVector(hit->Normal);
		Push(reflectedVelocty, true);
	}

}

void AAdaptativeMachine::CheckTrackProgress() {

	float progress = Spline->Spline->FindInputKeyClosestToWorldLocation(GetActorLocation())
		/ (Spline->Spline->GetNumberOfSplinePoints() - 1.f);
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("progress: %f"), progress));
	

	TrackManager->SetMachineProgress(ID, Lap + CurrentSegment + progress - Rank * 0.000001f);
	NetProgress = Lap + CurrentSegment + progress;
	RawProgress = Lap + CurrentSegment + progress - Rank * 0.000001f;
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Rank: %i"), Rank));
	if (progress == 1) {
		CurrentSegment += 1;
		Spline = TrackManager->GetNextSpline(CurrentSegment);
	}
}

void AAdaptativeMachine::CheckAvoidables(float deltaTime) {

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
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else if (NormalizedCurrentAvoidAmount < 0.75f) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = 0.25f;
			AlterPathTime = 0;
			BoostTime = 0;
			//NormalizedCurrentAvoidAmount = NormalizedDesiredAvoidAmount;
			AvoidDirection = -GetActorRightVector();
			bPrioritizeAvoidFalling = true;
		}
		//Down left
		traceStart = GetActorLocation() - GetActorRightVector() * 300 + GetActorUpVector() * 200 + GetActorForwardVector() * 200;
		traceEnd = traceStart - GetActorUpVector() * 1200.f;
		DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else if (NormalizedCurrentAvoidAmount < 0.75f) {

			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = 0.25f;
			AlterPathTime = 0;
			BoostTime = 0;
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
			AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
			if (otherMachine) {
				rightCollisionDistance *= 0.1f;
			}
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
			AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
			if (otherMachine) {
				leftCollisionDistance *= 0.1f;
			}
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("left distance: %f"), leftCollisionDistance));
		}
		else {
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}

		float delta = rightCollisionDistance - leftCollisionDistance;
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("delta ijo: %f"), delta));
		if (FMath::Abs(delta) > 0.0f) {
			NormalizedDesiredAvoidAmount = FMath::Max(rightCollisionDistance, leftCollisionDistance);
			AlterPathTime = 0;
			BoostTime = 0;
			AvoidDirection = GetActorRightVector() * FMath::Sign(delta);
		}
		else {
			NormalizedDesiredAvoidAmount = 0;
			AlterPathTime += deltaTime;
			BoostTime += deltaTime;
		}
	}
}

void AAdaptativeMachine::CheckForAlterPath() {
	if (AlterPathTime > CurrentAlterPathTimeThreshold) {
		AlterPathTime = 0;
		CurrentAlterPathTimeThreshold = FMath::RandRange(2.5f, 5.f);
		float rand = FMath::RandRange(-1.f, 1.f);
		AvoidDirection = GetActorRightVector() * FMath::Sign(rand);
		NormalizedDesiredAvoidAmount = FMath::Abs(rand) * 0.5f;
	}
}

void AAdaptativeMachine::CheckForBoost() {

	if (BoostTime > CurrentBoostTimeThreshold && BoostSpeed == 0) {
		BoostTime = 0;
		float rand = FMath::FRand();
		if (rand < Condition * ((float)Rank / TrackManager->ActiveMachines)
		&& Condition > 0.01f) {
			BoostSpeed = Boost;
			Condition -= 0.05f;
		}
	}
}

void AAdaptativeMachine::SetDetour() {
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Airpitch: %s"), *GetActorLabel()));
	bDetourAvailable = true;
}

FVector AAdaptativeMachine::Push(FVector pushVelocity, bool bCalculateDamage) {
	FVector pushDirection = pushVelocity.GetSafeNormal();
	float pushMagnitude = pushVelocity.Size();

	float forwardVelocityProjection = FVector::DotProduct(GetActorForwardVector(), pushDirection);
	float newSpeed = forwardVelocityProjection * pushMagnitude;
	float deltaSpeed = Speed - newSpeed;
	Speed = newSpeed;

	float verticalVelocityProjection = FVector::DotProduct(GetActorUpVector(), pushDirection);
	float verticalSpeed = verticalVelocityProjection * pushMagnitude;

	FVector rawBounce = pushVelocity - GetActorForwardVector() * Speed - GetActorUpVector() * verticalSpeed;
	BounceDirection = rawBounce.GetSafeNormal();
	BounceSpeed = rawBounce.Size();
	BoostSpeed = 0;

	if (bCalculateDamage) {
		float shield = 0.75f;
		float dot = FVector::DotProduct(LastDeltaLocation, pushVelocity);
		float normalizer = 1 / (LastDeltaLocation.Size() * pushVelocity.Size());
		Condition -= (0.5f - dot * normalizer * 0.5f) * (1 - (shield));
		if (Condition < 0) {
			SoftDestroy();
		}
	}

	if (bGrounded) {
		SurfaceAtractionSpeed = 0;
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Aplastado: %s, : %f"), *GetActorNameOrLabel(), verticalSpeed));
		//return GetActorUpVector() * FMath::Abs(verticalSpeed);
		//return GetActorUpVector() * 4000;
		//Speed += 1000; // FMath::Abs(verticalSpeed);
		return GetActorUpVector() * 2000 - GetActorForwardVector() * 10000;
		//return FVector::Zero();
	}
	else {
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Aventado: %s, : %f"), *GetActorNameOrLabel(), verticalSpeed));
		SurfaceAtractionSpeed = -verticalSpeed;
		return FVector::Zero();
	}
}

void AAdaptativeMachine::SetupTrackManager(UTrackManager* inTrackManager, int inID) {
	TrackManager = inTrackManager;
	ID = inID;
	Spline = TrackManager->Splines[0];
}

void AAdaptativeMachine::SetRank(int inRank) {
	Rank = inRank;
	CurrentBoostTimeThreshold = TrackManager->ActiveMachines - Rank;
}

void AAdaptativeMachine::SoftDestroy() {
	MaxSpeed = 0;
	Boost = 0;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bDepleted = true;
	TrackManager->ReportDisabledMachine();
}

void AAdaptativeMachine::CheckStuck(float deltaTime, FVector initialLocation) {
	LastDeltaLocation = (GetActorLocation() - initialLocation) / deltaTime;
	if (LastDeltaLocation.Size() == 0) {
		if (StuckedTime > 0) {
			if (!bStucked) {
				bStucked = true;
			}
			if (StuckedTime > 5) {
				SoftDestroy();
			}
		}
		StuckedTime += deltaTime;
	}
	else if (StuckedTime > 0) {
		StuckedTime = 0;
		bStucked = false;
	}
}

void AAdaptativeMachine::UpdateVisibleRotation(float deltaTime) {
	FRotator desiredVisibleRotation = FRotationMatrix::MakeFromXZ(MachineDirection, GetActorUpVector()).Rotator();
	VisibleRotation = FMath::Lerp(VisibleRotation, desiredVisibleRotation, deltaTime * VisibleRotationSpeed);
	VisibleComponent->SetWorldRotation(VisibleRotation);
}