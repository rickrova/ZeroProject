#include "AdaptativeMachine.h"
#include "PlayerMachine.h"

AAdaptativeMachine::AAdaptativeMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionComponent"));
	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));
	TargetOrientationArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("TargetOrientationArrowComponent"));
	InitialOrientationArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("InitialOrientationArrowComponent"));

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	FCollisionResponseContainer responseContainer = FCollisionResponseContainer(ECollisionResponse::ECR_Ignore);
	responseContainer.WorldStatic = ECollisionResponse::ECR_Block;
	responseContainer.PhysicsBody = ECollisionResponse::ECR_Block;
	responseContainer.Vehicle = ECollisionResponse::ECR_Block;
	responseContainer.Destructible = ECollisionResponse::ECR_Overlap;
	CollisionComponent->SetCollisionResponseToChannels(responseContainer);
	CollisionComponent->SetVisibility(false);

	VisibleComponent->SetupAttachment(CollisionComponent);
	VisibleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisibleComponent->SetGenerateOverlapEvents(false);
	VisibleComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	TargetOrientationArrowComponent->SetupAttachment(CollisionComponent);
	InitialOrientationArrowComponent->SetupAttachment(CollisionComponent);
}

void AAdaptativeMachine::BeginPlay()
{
	Super::BeginPlay();

	//AlignToSurface(0);
	//TimerDelegate.BindUFunction(this, FName("SetDetour"));
	//bDetourAvailable = true;
	bCanFindSurface = true;
	bStucked = false;
	//int rand = FMath::RandRange(0, 5);
	//GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 5, false, 5);

	VisibleRotationSpeed = 10;
	VisibleRotation = GetActorRotation();

	Condition = 1.f;
	CurrentAlterPathTimeThreshold = FMath::RandRange(0.25f, 3.f);
	CurrentRestorePathTimeThreshold = FMath::RandRange(2.5f, 3.f);
	CurrentBoostTimeThreshold = 3.f;
	CurrentSteering = DrivingSteering;
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
		CheckEnergy(DeltaTime);
		CheckStuck(DeltaTime, initialLocation);
		UpdateVisibleRotation(DeltaTime);
	}
	INTState = (int)MachineState;
}

void AAdaptativeMachine::ComputeDirection(float deltaTime) {

	// TrackDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(),
	// 	ESplineCoordinateSpace::World);

	float rawAvoidDelta = (NormalizedDesiredAvoidAmount + NormalizedDesiredDriveAmount) - NormalizedCurrentAvoidAmount;
	float normalizedAvoidDelta = FMath::Min(deltaTime * CurrentSteering, FMath::Abs(rawAvoidDelta)) * FMath::Sign(rawAvoidDelta);

	NormalizedCurrentAvoidAmount = FMath::Clamp(NormalizedCurrentAvoidAmount + normalizedAvoidDelta, -1, 1);
	if (NormalizedCurrentAvoidAmount == NormalizedDesiredAvoidAmount + NormalizedDesiredDriveAmount
		&& normalizedAvoidDelta != 0) {
		if (NormalizedDesiredAvoidAmount == 0 && NormalizedDesiredDriveAmount == 0) {
			CurrentSteering = DrivingSteering;
			AlterPathTime = 0;
			RestorePathTime = 0;
			CurrentAlterPathTimeThreshold = FMath::RandRange(2.5f, 5.f);
			MachineState = State::STRAIGHT;
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("start cycle")));
		}
		/*if (NormalizedDesiredDriveAmount == 0) {
			AlterPathTime = 0;
			RestorePathTime = 0;
			CurrentAlterPathTimeThreshold = FMath::RandRange(2.5f, 5.f);
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("start cycle")));
		}*/
		if (NormalizedDesiredDriveAmount != 0) {
			//NormalizedDesiredDriveAmount = 0;
			RestorePathTime = 0;
			CurrentRestorePathTimeThreshold = FMath::RandRange(2.5f, 5.f);
		}
	}
	FVector halfInterpolation = FMath::Lerp(GetActorForwardVector(), GetActorRightVector(), NormalizedCurrentAvoidAmount);
	FVector fullInterpolation = FMath::Lerp(halfInterpolation, -GetActorRightVector(), -NormalizedCurrentAvoidAmount);
	MachineDirection = fullInterpolation.GetSafeNormal();
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
	if (Speed < MaxSpeed && bCanRace) {
		Speed += AccelerationRate * deltaTime;
	}
	else if(bCanRace) {
		Speed -= AccelerationRate * deltaTime;
	}
	if (BoostRemainingTime > 0 && bCanRace) {
		BoostRemainingTime -= deltaTime;
		Condition -= deltaTime * 0.5f;
		BoostSpeed += BoostAccelerationRate * deltaTime;
		if (BoostRemainingTime < 0 || Condition <= deltaTime) {
			BoostRemainingTime = 0;
		}
	}
	if (ExternalBoostRemainingTime > 0 && bCanRace) {
		ExternalBoostRemainingTime -= deltaTime;
		BoostSpeed += BoostAccelerationRate * deltaTime;
		if (ExternalBoostRemainingTime < 0) {
			ExternalBoostRemainingTime = 0;
		}
	}
	if (BoostRemainingTime == 0 && ExternalBoostRemainingTime == 0 && BoostSpeed > 0 && bCanRace) {
		BoostSpeed -= BoostDeccelerationRate * deltaTime;
		if (BoostSpeed < 0) {
			BoostSpeed = 0;
		}
	}
	if (bCanRace && bGrounded && BoostRemainingTime == 0 && Condition < 1) {
		Condition += deltaTime * 0.01f;
		if (Condition > 1) {
			Condition = 1;
		}
	}
	if (!bGrounded) {
		SurfaceAtractionSpeed += SurfaceAtractionForce * deltaTime;
		MagneticDirection -= SurfaceNormal * SurfaceAtractionForce * deltaTime;
	}
	else if (MagneticDirection != FVector::ZeroVector) {
		MagneticDirection = FVector::ZeroVector;
	}
	if (BounceSpeed > 0) {
		BounceSpeed -= BounceDeccelerationRate * deltaTime;
	}
	else {
		BounceSpeed += BounceDeccelerationRate * deltaTime;
	}

	if (bMagneticZone) {
		MagneticDirection = MagneticSpline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World)
			- GetActorLocation();
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("magnetic vector: x: %f, y: %f, z: %f"),
			//MagneticDirection.X, MagneticDirection.Y, MagneticDirection.Z));
		float desiredSize = (5000 - FMath::Clamp(MagneticDirection.Size() * 1.f, 0, 5000)) / 5000.f;
		MagneticDirection.Normalize();
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("size: %f"), desiredSize));
		MagneticDirection *= desiredSize * deltaTime * 100000.f;
	}
	else {
		MagneticDirection = FVector::ZeroVector;
	}

	FVector desiredDeltaLocation = MachineDirection * (Speed + BoostSpeed) * deltaTime
		- SurfaceNormal * SurfaceAtractionSpeed * deltaTime
		+ MagneticDirection * deltaTime
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
		Bounce(hit, desiredDeltaLocation / deltaTime);
	}
	CurrentCollisionMachine = NULL;

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
	TrackDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);
	ClosestSplinePoint = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);

	if (!bJumping && GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
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
			NormalizedCurrentAvoidAmount = 0;
			NormalizedDesiredAvoidAmount = 0;
			NormalizedDesiredDriveAmount = 0;
			CurrentSteering = Steering;
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
		if (bJumping && SurfaceAtractionSpeed > 0) {
			bJumping = false;
		}
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() - SurfaceNormal * 500, FColor(255, 255, 0), false, 0.025f, 0, 10);
	}
	
	if(FVector::Distance(GetActorLocation(), ClosestSplinePoint) > 100000){
		SoftDestroy(TEXT("out of track"));
	}
}

void AAdaptativeMachine::ComputeClosestSurfaceNormal(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation();
	FVector traceEnd = ClosestSplinePoint;
	//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 255, 0), false, 0.025f, 0, 10);
	

	if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {

		//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, 0.025f, 0, 10);
		if (FVector::DotProduct(GetActorUpVector(), hit->Normal) < 0) {
			SurfaceNormal = hit->Normal;
		}
		else {
			SurfaceNormal = FMath::Lerp(SurfaceNormal, hit->Normal, deltaTime * 2.f);
		}
	}
	else {
		traceStart = traceEnd;
		FVector dir = (GetActorLocation() - traceStart).GetSafeNormal();
		traceEnd = traceStart + dir * 15000;
		//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 128, 0), false, 0.025f, 0, 10);

		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
			SurfaceNormal = hit->Normal;
		}
		else {
			//SoftDestroy();
			
			//SurfaceNormal = dir;
			//bCanFindSurface = false;
		}
	}
}

void AAdaptativeMachine::Bounce(FHitResult* hit, FVector deltaLocation) {
	AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
	
	if (otherMachine) {
		CurrentCollisionMachine = otherMachine;
		if (bStucked || otherMachine->bStucked) {
			FVector reflectedVelocty = deltaLocation.MirrorByVector(hit->Normal) * 100;
			otherMachine->Push(reflectedVelocty, true);
			Push(reflectedVelocty, true);
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
		APlayerMachine* playerMachine = Cast<APlayerMachine>(hit->GetActor());

		if (playerMachine) {
			//CurrentCollisionMachine = otherMachine;
			if (bStucked) {
				FVector reflectedVelocty = deltaLocation.MirrorByVector(hit->Normal) * 100;
				playerMachine->Push(reflectedVelocty, true);
				Push(reflectedVelocty, true);
			}
			else {
				FVector residualVerticalBounce = playerMachine->Push(LastDeltaLocation, true);
				if (bGrounded) {
					residualVerticalBounce = FVector::ZeroVector;
				}
				Push(playerMachine->LastDeltaLocation + residualVerticalBounce, true);
			}
		}
		else {
			FVector impactDirection = (GetActorLocation() - hit->ImpactPoint).GetSafeNormal();
			FVector reflectedVelocty = LastDeltaLocation.MirrorByVector(impactDirection); // hit->ImpactNormal);
			if (bStucked) {
				reflectedVelocty = deltaLocation.MirrorByVector(impactDirection) * 100; // impactDirection * 500.f;
			}
			Push(reflectedVelocty, true);
		}
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

	NormalizedDesiredAvoidAmount *= 0.5f;

	//Check for risk of falling
	if (bGrounded) {
		//Down right
		traceStart = GetActorLocation() + GetActorRightVector() * 300 + GetActorUpVector() * 200 + GetActorForwardVector() * 200;
		traceEnd = traceStart - GetActorUpVector() * 1200.f;
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = -1.f;
			NormalizedDesiredDriveAmount = 0;
			CurrentSteering = Steering;
			AlterPathTime = 0;
			RestorePathTime = 0;
			BoostTime = 0;
			//NormalizedCurrentAvoidAmount = NormalizedDesiredAvoidAmount;
			//AvoidDirection = -GetActorRightVector();
			bPrioritizeAvoidFalling = true;
			MachineState = State::EVADING;
		}
		//Down left
		traceStart = GetActorLocation() - GetActorRightVector() * 300 + GetActorUpVector() * 200 + GetActorForwardVector() * 200;
		traceEnd = traceStart - GetActorUpVector() * 1200.f;
		//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		else if (!bPrioritizeAvoidFalling) {

			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			NormalizedDesiredAvoidAmount = 1.f;
			NormalizedDesiredDriveAmount = 0;
			CurrentSteering = Steering;
			AlterPathTime = 0;
			RestorePathTime = 0;
			BoostTime = 0;
			//NormalizedCurrentAvoidAmount = NormalizedDesiredAvoidAmount;
			//AvoidDirection = GetActorRightVector();
			bPrioritizeAvoidFalling = true;
			MachineState = State::EVADING;
		}
		else {
			bPrioritizeAvoidFalling = false;
			MachineState = State::STRAIGHT;
		}

		if (!bPrioritizeAvoidFalling && !bCanFindSurface) {
			bCanFindSurface = true;
		}
	}
	else{
		traceStart = GetActorLocation();
		traceEnd = traceStart - SurfaceNormal * 15000;
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 255, 0), false, 0.025f, 0, 10);

			bPrioritizeAvoidFalling = false;
			MachineState = State::STRAIGHT;
		}
		else {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, 0.025f, 0, 10);
			FVector safeDirection = (ClosestSplinePoint - traceStart).GetSafeNormal();
			float side = FVector::DotProduct(safeDirection, GetActorRightVector());
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + GetActorRightVector() * side * 500.f, FColor(0, 0, 255), false, 0.025f, 0, 20);
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + GetActorRightVector() * 500.f, FColor(255, 0, 255), false, 0.025f, 0, 10);
			NormalizedDesiredAvoidAmount = side;
			NormalizedDesiredDriveAmount = 0;
			CurrentSteering = Steering;
			AlterPathTime = 0;
			RestorePathTime = 0;
			BoostTime = 0;
			//NormalizedCurrentAvoidAmount = NormalizedDesiredAvoidAmount;
			//AvoidDirection = GetActorRightVector();
			bPrioritizeAvoidFalling = true;
			MachineState = State::EVADING;
		}
	}

	//Check for collision
	if (MachineState != State::EVADING) {
		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(this);
		//FVector flatDirection = FVector::VectorPlaneProject(MachineDirection, GetActorUpVector()).GetSafeNormal();
		FVector flatRight = FVector::CrossProduct(MachineDirection, GetActorUpVector());

		//Front right
		traceStart = GetActorLocation() - MachineDirection * 200 + flatRight * 300; // *(Speed / MaxSpeed);
		traceEnd = GetActorLocation() + MachineDirection * 8000.f; // *(Speed / MaxSpeed);
		float traceDistance = FVector::Distance(traceStart, traceEnd);
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_Vehicle, queryParams)) {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			rightCollisionDistance = (traceDistance - hit->Distance) / traceDistance;
			AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
			if (otherMachine) {
				if (otherMachine->bStucked) {
					rightCollisionDistance *= 0.9f;
					CurrentSteering = Steering;
				}else{
					rightCollisionDistance *= 0.15f;
					CurrentSteering = DrivingSteering;
				}
			}
			else {
				APlayerMachine* playerMachine = Cast<APlayerMachine>(hit->GetActor());
				if (playerMachine) {
					rightCollisionDistance *= 0.15f;
					CurrentSteering = DrivingSteering;
				}
				else {
					if (hit->GetComponent()->GetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody) == ECollisionResponse::ECR_Block) {
						rightCollisionDistance *= 0.5f;
						CurrentSteering = Steering;
					}
					else {
						rightCollisionDistance *= 0.15f;
						CurrentSteering = DrivingSteering;
					}
				}
			}
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("right distance: %f"), rightCollisionDistance));
		}
		else {

			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}
		//Front left
		traceStart = GetActorLocation() - MachineDirection * 200 - flatRight * 300; // *(Speed / MaxSpeed);
		traceEnd = GetActorLocation() + MachineDirection * 8000.f; // *(Speed / MaxSpeed);
		if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_Vehicle, queryParams)) {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(255, 0, 0), false, -1, 0, 10);
			leftCollisionDistance = (traceDistance - hit->Distance) / traceDistance;
			AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());
			if (otherMachine) {
				if (otherMachine->bStucked) {
					leftCollisionDistance *= 0.9f;
					CurrentSteering = Steering;
				}else{
					leftCollisionDistance *= 0.15f;
					CurrentSteering = DrivingSteering;
				}
			}
			else {
				APlayerMachine* playerMachine = Cast<APlayerMachine>(hit->GetActor());
				if (playerMachine) {
					leftCollisionDistance *= 0.15f;
					CurrentSteering = DrivingSteering;
				}
				else {
					if (hit->GetComponent()->GetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody) == ECollisionResponse::ECR_Block) {
						leftCollisionDistance *= 0.5f;
						CurrentSteering = Steering;
					}
					else {
						leftCollisionDistance *= 0.15f;
						CurrentSteering = DrivingSteering;
					}
				}
			}
			//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("left distance: %f"), leftCollisionDistance));
		}
		else {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}

		float delta = rightCollisionDistance - leftCollisionDistance;
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("delta ijo: %f"), delta));
		if (FMath::Abs(delta) > 0.0f) {
			NormalizedDesiredAvoidAmount = FMath::Sign(delta); //FMath::Max(rightCollisionDistance, leftCollisionDistance) * FMath::Sign(delta);
			NormalizedDesiredDriveAmount = 0;
			//CurrentSteering = Steering;
			bInstantSteer = true;
			AlterPathTime = 0;
			RestorePathTime = 0;
			BoostTime = 0;
			if (MachineState != State::DEVIATION) {
				MachineState = State::DEVIATION;
			}
			//AvoidDirection = GetActorRightVector() * FMath::Sign(delta);
		}
		else {
			if(MachineState == State::STRAIGHT){
				AlterPathTime += deltaTime;
			}
			else if (MachineState == State::DRIVING) {
				RestorePathTime += deltaTime;
			}
			BoostTime += deltaTime;
			/*if (MachineState != State::STRAIGHT) {
				MachineState = State::STRAIGHT;
			}*/
		}

		float brakeThreshold = 0.0f;
		if (rightCollisionDistance > brakeThreshold && leftCollisionDistance > brakeThreshold && Speed > MaxSpeed * 0.25f) {
			Speed -= AccelerationRate * deltaTime + AccelerationRate * deltaTime * (FMath::Min(rightCollisionDistance, leftCollisionDistance) - brakeThreshold);
		}
	}
}

void AAdaptativeMachine::CheckForAlterPath() {
	if (AlterPathTime > CurrentAlterPathTimeThreshold && MachineState == State::STRAIGHT) {
		FVector directionToTrackCenter = (ClosestSplinePoint - GetActorLocation()).GetSafeNormal();
		float alignToCenterFactor = FVector::DotProduct(directionToTrackCenter, GetActorRightVector()) * 0.1f;
		AlterPathTime = 0;
		//CurrentAlterPathTimeThreshold = 1000.f;
		bool bNegative = FMath::RandBool();
		float randFloat = FMath::RandRange(0.f, 0.1f);
		//AvoidDirection = GetActorRightVector() * FMath::Sign(bNegative ? 1 : -1);
		NormalizedDesiredDriveAmount = randFloat * FMath::Sign(bNegative ? 1 : -1) + alignToCenterFactor;
		MachineState = State::DRIVING;
		//CurrentSteering = DrivingSteering;
		//GEngine->AddOnScreenDebugMessage(-1, 100.f, FColor::Yellow, FString::Printf(TEXT("AlterPath: %s: %f"), *GetActorLabel(), NormalizedDesiredAvoidAmount));
	}else if (RestorePathTime > CurrentRestorePathTimeThreshold && MachineState == State::DRIVING) {
		RestorePathTime = 0;
		//CurrentRestorePathTimeThreshold = 1000.f;
		NormalizedDesiredDriveAmount = 0.f;
	}
}

void AAdaptativeMachine::CheckForBoost() {

	if (BoostTime > CurrentBoostTimeThreshold && BoostSpeed == 0 && Condition > 0.5f && bCanRace) {
		BoostTime = 0;
		float rand = FMath::FRand();
		//if (rand < Condition * ((float)Rank / TrackManager->ActiveMachines) * 0.25f) {
		if (rand < Condition * 0.1f) {
			//GEngine->AddOnScreenDebugMessage(-1, 100.f, FColor::Yellow, FString::Printf(TEXT("Boost machine: %s"), *GetActorNameOrLabel()));
			//BoostSpeed = Boost;
			//Condition -= 0.025f;

			BoostRemainingTime = BoostDurationTime;
		}
	}
}

void AAdaptativeMachine::CheckEnergy(float deltaTime) {
	if (bEnergyTransmission) {
		Condition += EnergyTransmissionRatio * deltaTime;
		if (Condition > 1) {
			Condition = 1;
		}
		else if (Condition <= 0) {
			Condition = 0;
			SoftDestroy(TEXT("energy depleted"));
		}
	}
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
	NormalizedCurrentAvoidAmount = 0;
	NormalizedDesiredAvoidAmount = 0;
	NormalizedDesiredDriveAmount = 0;

	if (bCalculateDamage) {
		float shield = 0.75f;
		float dot = FVector::DotProduct(LastDeltaLocation.GetSafeNormal(), pushVelocity.GetSafeNormal());
		float normalizer = FVector::Distance(LastDeltaLocation, pushVelocity) / MaxSpeed;
		float damage = (0.5f - dot * 0.5f) * normalizer * (1.f - (shield));
		float oldCondition = Condition;
		Condition -= damage;
		if (Condition < 0) {
			SoftDestroy(TEXT("energy depleted"));
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

void AAdaptativeMachine::SetupTrackManager(ATrackManager_v2* inTrackManager, int inID) {
	TrackManager = inTrackManager;
	ID = inID;
	Spline = TrackManager->Splines[0];
}

void AAdaptativeMachine::SetRank(int inRank) {
	Rank = inRank;
	CurrentBoostTimeThreshold = 3.f; // (TrackManager->ActiveMachines - Rank + 1) / 4;
}

void AAdaptativeMachine::SoftDestroy(FString inText) {
	Disable();
	TrackManager->ReportDisabledMachine();
}

void AAdaptativeMachine::Disable() {
	MaxSpeed = 0;
	BoostRemainingTime = 0;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bDepleted = true;
}

void AAdaptativeMachine::ExternalBoost(float inBoostRatio) {
	ExternalBoostRemainingTime = inBoostRatio;
}

void AAdaptativeMachine::Jump(float inJumpRatio) {
	bGrounded = false;
	bJumping = true;
	SurfaceAtractionSpeed = inJumpRatio;
}

void AAdaptativeMachine::Slow(float inSlowRatio) {
	MaxSpeed *= inSlowRatio;
	AccelerationRate /= inSlowRatio;
	//Speed *= (1 - inSlowRatio) / 2.f;
}

void AAdaptativeMachine::ResetSlow(float inSlowRatio) {
	MaxSpeed /= inSlowRatio;
	AccelerationRate *= inSlowRatio;
}

void AAdaptativeMachine::StartEnergyTransmission(float inEnergy) {
	EnergyTransmissionRatio = inEnergy;
	bEnergyTransmission = true;
}

void AAdaptativeMachine::EndEnergyTransmission() {
	bEnergyTransmission = false;
}

void AAdaptativeMachine::StartMagnetic(USplineComponent* inSpline) {
	MagneticSpline = inSpline;
	bMagneticZone = true;
}

void AAdaptativeMachine::EndMagnetic() {
	bMagneticZone = false;
}

void AAdaptativeMachine::CheckStuck(float deltaTime, FVector initialLocation) {
	LastDeltaLocation = (GetActorLocation() - initialLocation) / deltaTime;
	if (LastDeltaLocation.Size() == 0 && bCanRace) {
		if (StuckedTime > 1) {
			if (!bStucked) {
				bStucked = true;
				//GEngine->AddOnScreenDebugMessage(-1, 100, FColor::Red, FString::Printf(TEXT("machine %s stucked"), *GetActorNameOrLabel()));
			}
			if (StuckedTime > 3) {
				SoftDestroy(TEXT("stucked too long"));
			}
		}
		StuckedTime += deltaTime;
	}
	else if (StuckedTime > 0 && bCanRace) {
		StuckedTime = 0;
		bStucked = false;
	}
}

void AAdaptativeMachine::UpdateVisibleRotation(float deltaTime) {
	TargetOrientationArrowComponent->SetWorldRotation(FRotationMatrix::MakeFromXZ(GetActorForwardVector(), GetActorUpVector()).Rotator());
	InitialOrientationArrowComponent->SetWorldRotation(VisibleRotation);
	FRotator tempRotation = FMath::Lerp(InitialOrientationArrowComponent->GetRelativeRotation(),
		TargetOrientationArrowComponent->GetRelativeRotation(), deltaTime * VisibleRotationSpeed);
	VisibleComponent->SetRelativeRotation(tempRotation);
	VisibleRotation = VisibleComponent->GetComponentRotation();


	//FRotator desiredVisibleRotation = FRotator::ZeroRotator; // FRotationMatrix::MakeFromXZ(MachineDirection, GetActorUpVector()).Rotator();
	//VisibleRotation = FMath::Lerp(FRotator::ZeroRotator, desiredVisibleRotation, deltaTime * 10); // VisibleRotationSpeed);
	//VisibleComponent->SetWorldRotation(VisibleRotation);
}