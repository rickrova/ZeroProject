#include "PlayerMachine.h"
#include "SplineActor.h"
#include "AdaptativeMachine.h"
#include "GameFramework/SpringArmComponent.h"

APlayerMachine::APlayerMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionComponent"));
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	ArrowComponentDos = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponentDos"));

	VisibleComponent->SetupAttachment(CollisionComponent);
	ArrowComponent->SetupAttachment(CollisionComponent);
	CameraComponent->SetupAttachment(CollisionComponent);
	ArrowComponentDos->SetupAttachment(CollisionComponent);

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

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void APlayerMachine::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis("MoveX", this, &APlayerMachine::MoveRight);
	InputComponent->BindAxis("MoveY", this, &APlayerMachine::MoveForward);
	InputComponent->BindAxis("GripRight", this, &APlayerMachine::GripRight);
	InputComponent->BindAxis("GripLeft", this, &APlayerMachine::GripLeft);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Pressed, this, &APlayerMachine::Accelerate);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Released, this, &APlayerMachine::Deccelerate);
	InputComponent->BindAction("Brake", EInputEvent::IE_Pressed, this, &APlayerMachine::Brake);
	InputComponent->BindAction("Brake", EInputEvent::IE_Released, this, &APlayerMachine::LiftBrake);
	InputComponent->BindAction("Boost", EInputEvent::IE_Pressed, this, &APlayerMachine::ExpelBoost);
	InputComponent->BindAction("Pause", EInputEvent::IE_Pressed, this, &APlayerMachine::Pause).bExecuteWhenPaused = true;
}

void APlayerMachine::BeginPlay()
{
	Super::BeginPlay();
	bCanFindSurface = true;
	bStucked = false;
	VisibleRotationSpeed = 10;
	VisibleRotation = GetActorRotation();
	CameraRotation = VisibleRotation;
	ArrowComponent->SetWorldRotation(VisibleRotation);
	MachineDirection = GetActorForwardVector();
	Condition = 1.f;
	CurrentAlterPathTimeThreshold = FMath::RandRange(0.25f, 3.f);
	CurrentRestorePathTimeThreshold = FMath::RandRange(2.5f, 3.f);
	CurrentBoostTimeThreshold = 1.f;
	CurrentSteering = DrivingSteering;
	AlignToSurface(0);
	MagnetState = MagnetState::ENGAGED;
}

void APlayerMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void APlayerMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bDepleted) {
		FVector initialLocation = GetActorLocation();
		ComputeDirection(DeltaTime);
		ComputeMovement(DeltaTime);
		CheckTrackProgress();
		AlignToSurface(DeltaTime);
		LastDeltaLocation = (GetActorLocation() - initialLocation) / DeltaTime;
		//CheckAvoidables(DeltaTime);
		//CheckForAlterPath();
		//CheckForBoost();
		CheckEnergy(DeltaTime);
		//CheckStuck(DeltaTime, initialLocation);
		UpdateVisibleRotation(DeltaTime);
	}
	INTState = (int)MachineState;
}

void APlayerMachine::ComputeDirection(float deltaTime) {/*
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
		}
		if (NormalizedDesiredDriveAmount != 0) {
			RestorePathTime = 0;
			CurrentRestorePathTimeThreshold = FMath::RandRange(2.5f, 5.f);
		}
	}
	FVector halfInterpolation = FMath::Lerp(GetActorForwardVector(), GetActorRightVector(), NormalizedCurrentAvoidAmount);
	FVector fullInterpolation = FMath::Lerp(halfInterpolation, -GetActorRightVector(), -NormalizedCurrentAvoidAmount);*/
	FRotator deltaRotation = FRotator::ZeroRotator;
	ArrowComponent->SetRelativeRotation(FRotator::ZeroRotator);

	float driftRisk = RightGrip + LeftGrip;
	if (!bDoubleGripOnDrift && driftRisk > GripInputMultiplier * 2 - 0.05f) {
		bDoubleGripOnDrift = true;
	}
	else if (bDoubleGripOnDrift && driftRisk < GripInputMultiplier * 2) {
		bDoubleGripOnDrift = false;
	}
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("drift input: %f"), driftRisk));
	MovementInput.X += RightGrip - LeftGrip;
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("movementInput input: %f"), MovementInput.X));
	float deltaX = MovementInput.X - LastInputX;
	LastInputX = MovementInput.X;
	if (MagnetState == MagnetState::ENGAGED && bGrounded && (driftRisk > GripInputMultiplier * 2 - 0.05f
		|| FMath::Abs(MovementInput.X) > Grip || FMath::Abs(deltaX) * (Speed + BoostSpeed) / MaxSpeed > InputDriftThereshold)) {
		MagnetState = MagnetState::DRIFT;
	}

	if (MagnetState == MagnetState::ENGAGED && !MovementInput.IsZero() && bCanFindSurface)
	{
		deltaRotation.Yaw = MovementInput.X * deltaTime * Steering;
	}
	else if (MagnetState == MagnetState::DRIFT && bCanFindSurface)
	{
		VirtualDesiredDriftYaw += MovementInput.X* deltaTime* Steering;
		VirtualDesiredDriftYaw = FMath::Clamp(VirtualDesiredDriftYaw, -10, 10);
		
		float gripOnDrift = 0.05f;
		if (bDoubleGripOnDrift) {
			gripOnDrift *= 2;
		}
		deltaRotation.Yaw = VirtualDesiredDriftYaw * gripOnDrift; //FMath::Lerp(0, VirtualDesiredDriftYaw, gripOnDrift);

		VirtualDesiredDriftYaw -= VirtualDesiredDriftYaw * 0.06f;

		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("desired drift yaw: %f"), VirtualDesiredDriftYaw));

		if (FMath::Abs(VirtualDesiredDriftYaw) < 0.05f && driftRisk == 0) {
			MagnetState = MagnetState::ENGAGED;
			bDoubleGripOnDrift = false;
		}
	}
	ArrowComponent->AddLocalRotation(deltaRotation);
	MachineDirection = ArrowComponent->GetForwardVector();
}

void APlayerMachine::ComputeMovement(float deltaTime) {
	float onAirSpeedModifier = 0;
	float onAirSurfaceAtractionSpeedModifier = 0;
	if (!bGrounded && bAccelerating) {
		onAirSpeedModifier = FMath::Clamp(MovementInput.Y, -1 , 0);
		onAirSurfaceAtractionSpeedModifier = FMath::Clamp(MovementInput.Y, 0, 1);
	}
	float onAirSpeedCalibration = 0.2f;
	if (MagnetState == MagnetState::ENGAGED && Speed <= MaxSpeed + MaxSpeed * onAirSpeedCalibration * onAirSpeedModifier
		&& bAccelerating && bCanRace && !bBraking) {
		Speed += AccelerationRate * deltaTime;
		if (Speed > MaxSpeed + MaxSpeed * onAirSpeedCalibration * onAirSpeedModifier) {
			Speed = MaxSpeed + MaxSpeed * onAirSpeedCalibration * onAirSpeedModifier;
		}
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, FString::Printf(TEXT("speed: %f"), Speed));
	}
	else if (MagnetState == MagnetState::DRIFT && Speed < MaxSpeed * (1 - LeftGrip * 0.1f - RightGrip * 0.1f)
		+ FMath::Abs(MovementInput.X) * MaxSpeed && bAccelerating && bCanRace && !bBraking) {
		Speed += (DriftingAccelerationRate * FMath::Abs(MovementInput.X) / (1 + GripInputMultiplier)
			+ AccelerationRate * onAirSpeedModifier * 2) * deltaTime;	
	}
	else if (MagnetState == MagnetState::DRIFT && Speed > MaxSpeed * (1 - LeftGrip * 0.1f - RightGrip * 0.1f)
		+ FMath::Abs(MovementInput.X) * MaxSpeed && bAccelerating && bCanRace && !bBraking) {
		Speed -= AccelerationRate * deltaTime;
	}
	else if (Speed > 0 && !bAccelerating && bCanRace && !bBraking){
		Speed -= DeccelerationRate * deltaTime;
		if (Speed < 0) {
			Speed = 0;
		}
	}
	else if (Speed > MaxSpeed + MaxSpeed * onAirSpeedCalibration * onAirSpeedModifier && bAccelerating && bCanRace && !bBraking) {
		Speed -= AccelerationRate * deltaTime;
		if (Speed < MaxSpeed + MaxSpeed * onAirSpeedCalibration * onAirSpeedModifier) {
			Speed = MaxSpeed + MaxSpeed * onAirSpeedCalibration * onAirSpeedModifier;
		}
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("speed: %f"), Speed));
	}else if (Speed > 0 && bBraking && bCanRace) {
		Speed -= BrakeDeccelerationRate * deltaTime;
		if (Speed < 0) {
			Speed = 0;
		}
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
	if (bCanRace && MagnetState == MagnetState::ENGAGED && bGrounded && BoostRemainingTime == 0 && Condition < 1) {
		Condition += deltaTime * 0.01f;
		if (Condition > 1) {
			Condition = 1;
		}
	}
	if (!bGrounded) {
		SurfaceAtractionSpeed += (SurfaceAtractionForce
			+ SurfaceAtractionForce * onAirSpeedModifier * (FMath::Clamp(Speed / MaxSpeed, 0 , MaxSpeed))
			+ AccelerationRate * onAirSurfaceAtractionSpeedModifier * 2) * deltaTime;
	}
	if (BounceSpeed > 0) {
		BounceSpeed -= BounceDeccelerationRate * deltaTime;
		if (BounceSpeed < 0) {
			BounceSpeed = 0;
		}
	}
	else if (BounceSpeed < 0) {
		BounceSpeed += BounceDeccelerationRate * deltaTime;
		if (BounceSpeed > 0) {
			BounceSpeed = 0;
		}
	}

	if (bMagneticZone) {
		MagneticDirection = MagneticSpline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World)
			- GetActorLocation();
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("magnetic vector: x: %f, y: %f, z: %f"),
			//MagneticDirection.X, MagneticDirection.Y, MagneticDirection.Z));
		float desiredSize = (5000 - FMath::Clamp(MagneticDirection.Size() * 0.5f, 0, 5000)) / 5000.f;
		MagneticDirection.Normalize();
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("size: %f"), desiredSize));
		MagneticDirection *= desiredSize * deltaTime * 500000.f;
	}
	else {
		MagneticDirection = FVector::ZeroVector;
	}

	FVector desiredDeltaLocation = MachineDirection * (Speed + BoostSpeed) * deltaTime
		- SurfaceNormal * SurfaceAtractionSpeed * deltaTime
		+ MagneticDirection * deltaTime
		+ BounceDirection * BounceSpeed * deltaTime;
	FHitResult* hit = new FHitResult();
	RootComponent->MoveComponent(desiredDeltaLocation, FRotationMatrix::MakeFromXZ(MachineDirection, SurfaceNormal).Rotator(),
		true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
	if (hit->bBlockingHit) {
		Bounce(hit, desiredDeltaLocation / deltaTime);
	}
	CurrentCollisionMachine = NULL;
}

void APlayerMachine::AlignToSurface(float deltaTime) {
	FHitResult* hit = new FHitResult();
	FVector traceStart = GetActorLocation() + GetActorUpVector() * TraceUpDistance;
	FVector traceEnd = traceStart - GetActorUpVector() * (TraceUpDistance + TraceDownDistance);
	TrackDirection = Spline->Spline->FindDirectionClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);
	ClosestSplinePoint = Spline->Spline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);

	if (!bJumping && GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECollisionChannel::ECC_WorldDynamic)) {
		SurfacePoint = hit->ImpactPoint;
		SurfaceNormal = hit->ImpactNormal;
		FVector deltaLocation = SurfacePoint + GetActorUpVector() * DistanceToSurface - GetActorLocation();
		RootComponent->MoveComponent(deltaLocation, FRotationMatrix::MakeFromZX(SurfaceNormal, MachineDirection).Rotator(),
			true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
		if (hit->bBlockingHit) {
		}
		if (!bGrounded) {
			bGrounded = true;
			StartShake(SurfaceAtractionSpeed * 0.02f);
			//CameraLocation = ArrowComponentDos->GetUpVector() * (500 - SurfaceAtractionSpeed * 0.025f)
			//	- ArrowComponent->GetForwardVector() * (1000 + SurfaceAtractionSpeed * 0.025f);
			SurfaceAtractionSpeed = 0;
			VisibleRotation = GetActorRotation();
			//VisibleRotationSpeed = 10;
		}
	}
	else if (bCanFindSurface) {
		ComputeClosestSurfaceNormal(deltaTime);
		SetActorRotation(FRotationMatrix::MakeFromZX(SurfaceNormal, MachineDirection).Rotator());
		if (bGrounded) {

			bGrounded = false;

			Push(LastDeltaLocation, false);
			VisibleRotationSpeed = 1.5f;
			MagnetState = MagnetState::ENGAGED;
		}
		if (bJumping && SurfaceAtractionSpeed > 0) {
			bJumping = false;
		}
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() - SurfaceNormal * 500, FColor(255, 255, 0), false, 0.025f, 0, 10);
	}

	if (FVector::Distance(GetActorLocation(), ClosestSplinePoint) > 1000000) {
		SoftDestroy();
	}
}

void APlayerMachine::ComputeClosestSurfaceNormal(float deltaTime) {
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
		//DrawDebugLine(GetWorld(), traceStart, traceStart - SurfaceNormal * 15000, FColor(255, 0, 0), false, 0.025f, 0, 10);
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
			//SurfaceNormal = dir;
			//bCanFindSurface = false;
		}
	}
}

void APlayerMachine::Bounce(FHitResult* hit, FVector deltaLocation) {
	AAdaptativeMachine* otherMachine = Cast<AAdaptativeMachine>(hit->GetActor());

	if (otherMachine) {
		//CurrentCollisionMachine = otherMachine;
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
			FVector residualVerticalBounce = playerMachine->Push(LastDeltaLocation, true);
			if (bGrounded) {
				residualVerticalBounce = FVector::ZeroVector;
			}
			Push(playerMachine->LastDeltaLocation + residualVerticalBounce, true);
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

void APlayerMachine::CheckTrackProgress() {

	float progress = Spline->Spline->FindInputKeyClosestToWorldLocation(GetActorLocation())
		/ (Spline->Spline->GetNumberOfSplinePoints() - 1.f);
	TrackManager->SetMachineProgress(ID, Lap + CurrentSegment + progress - Rank * 0.000001f);
	NetProgress = Lap + CurrentSegment + progress;
	RawProgress = Lap + CurrentSegment + progress - Rank * 0.000001f;
	if (progress == 1) {
		CurrentSegment += 1;
		Spline = TrackManager->GetNextSpline(CurrentSegment);
	}
}

void APlayerMachine::CheckAvoidables(float deltaTime) {

	FHitResult* hit = new FHitResult();
	float rightCollisionDistance = 0;
	float leftCollisionDistance = 0;
	bool bPrioritizeAvoidFalling = false;
	FVector traceStart;
	FVector traceEnd;

	//Check for risk of falling
	if (bGrounded) {
		NormalizedDesiredAvoidAmount *= 0.5f;
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

	//Check for collision
	if (MachineState != State::EVADING) {
		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(this);
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
				}
				else {
					rightCollisionDistance *= 0.15f;
					CurrentSteering = DrivingSteering;
				}
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
				}
				else {
					leftCollisionDistance *= 0.15f;
					CurrentSteering = DrivingSteering;
				}
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
		else {
			//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor(0, 255, 0), false, -1, 0, 10);
		}

		float delta = rightCollisionDistance - leftCollisionDistance;
		if (FMath::Abs(delta) > 0.0f) {
			NormalizedDesiredAvoidAmount = FMath::Sign(delta); //FMath::Max(rightCollisionDistance, leftCollisionDistance) * FMath::Sign(delta);
			NormalizedDesiredDriveAmount = 0;
			bInstantSteer = true;
			AlterPathTime = 0;
			RestorePathTime = 0;
			BoostTime = 0;
			if (MachineState != State::DEVIATION) {
				MachineState = State::DEVIATION;
			}
		}
		else {
			if (MachineState == State::STRAIGHT) {
				AlterPathTime += deltaTime;
			}
			else if (MachineState == State::DRIVING) {
				RestorePathTime += deltaTime;
			}
			BoostTime += deltaTime;
		}

		float brakeThreshold = 0.0f;
		if (rightCollisionDistance > brakeThreshold && leftCollisionDistance > brakeThreshold && Speed > MaxSpeed * 0.25f) {
			Speed -= AccelerationRate * deltaTime + AccelerationRate * deltaTime * (FMath::Min(rightCollisionDistance, leftCollisionDistance) - brakeThreshold);
		}
	}
}

void APlayerMachine::CheckForAlterPath() {
	if (AlterPathTime > CurrentAlterPathTimeThreshold && MachineState == State::STRAIGHT) {
		FVector directionToTrackCenter = (ClosestSplinePoint - GetActorLocation()).GetSafeNormal();
		float alignToCenterFactor = FVector::DotProduct(directionToTrackCenter, GetActorRightVector()) * 0.1f;
		AlterPathTime = 0;
		bool bNegative = FMath::RandBool();
		float randFloat = FMath::RandRange(0.f, 0.1f);
		NormalizedDesiredDriveAmount = randFloat * FMath::Sign(bNegative ? 1 : -1) + alignToCenterFactor;
		MachineState = State::DRIVING;
	}
	else if (RestorePathTime > CurrentRestorePathTimeThreshold && MachineState == State::DRIVING) {
		RestorePathTime = 0;
		NormalizedDesiredDriveAmount = 0.f;
	}
}

void APlayerMachine::CheckForBoost() {

	if (BoostTime > CurrentBoostTimeThreshold && BoostSpeed == 0 && Condition > 0.5f) {
		BoostTime = 0;
		float rand = FMath::FRand();
		if (rand < Condition * ((float)Rank / TrackManager->ActiveMachines) * 0.25f) {
			//BoostSpeed = Boost;
			Condition -= 0.025f;
		}
	}
}

void APlayerMachine::CheckEnergy(float deltaTime) {
	if (bEnergyTransmission) {
		Condition += EnergyTransmissionRatio * deltaTime;
		if (Condition > 1) {
			Condition = 1;
		}
		else if (Condition <= 0) {
			Condition = 0;
			SoftDestroy();
		}
	}
}

FVector APlayerMachine::Push(FVector pushVelocity, bool bCalculateDamage) {
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
			SoftDestroy();
		}
	}

	if (bGrounded) {
		SurfaceAtractionSpeed = 0;
		return GetActorUpVector() * 2000 - GetActorForwardVector() * 10000;
	}
	else {
		SurfaceAtractionSpeed = -verticalSpeed;
		return FVector::Zero();
	}
}

void APlayerMachine::SetupTrackManager(ATrackManager_v2* inTrackManager, int inID) {
	TrackManager = inTrackManager;
	ID = inID;
	Spline = TrackManager->Splines[0];
}

void APlayerMachine::SetRank(int inRank) {
	Rank = inRank;
	CurrentBoostTimeThreshold = TrackManager->ActiveMachines - Rank + 1;
}

void APlayerMachine::SoftDestroy() {
	Disable();
	TrackManager->ReportDisabledMachine();
}

void APlayerMachine::Disable() {
	MaxSpeed = 0;
	BoostRemainingTime = 0;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bDepleted = true;
}

void APlayerMachine::ExternalBoost(float inBoostRatio) {
	ExternalBoostRemainingTime = inBoostRatio;
}

void APlayerMachine::Jump(float inJumpRatio) {
	bGrounded = false;
	bJumping = true;
	SurfaceAtractionSpeed = inJumpRatio;
}

void APlayerMachine::Slow(float inSlowRatio) {
	MaxSpeed *= inSlowRatio;
	//Speed *= (1 - inSlowRatio) / 2.f;
	AccelerationRate /= inSlowRatio;
}

void APlayerMachine::ResetSlow(float inSlowRatio) {
	MaxSpeed /= inSlowRatio;
	AccelerationRate *= inSlowRatio;
}

void APlayerMachine::StartEnergyTransmission(float inEnergy) {
	EnergyTransmissionRatio = inEnergy;
	bEnergyTransmission = true;
}

void APlayerMachine::EndEnergyTransmission() {
	bEnergyTransmission = false;
}

void APlayerMachine::StartMagnetic(USplineComponent * inSpline) {
	MagneticSpline = inSpline;
	bMagneticZone = true;
}

void APlayerMachine::EndMagnetic() {
	bMagneticZone = false;
}

void APlayerMachine::CheckStuck(float deltaTime, FVector initialLocation) {
	LastDeltaLocation = (GetActorLocation() - initialLocation) / deltaTime;
	if (LastDeltaLocation.Size() == 0) {
		if (StuckedTime > 1) {
			if (!bStucked) {
				bStucked = true;
			}
			if (StuckedTime > 3) {
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

void APlayerMachine::UpdateVisibleRotation(float deltaTime) {
	FVector desiredZ = GetActorUpVector();
	SpeedModifier = FMath::Lerp(SpeedModifier, (Speed + SurfaceAtractionSpeed) / MaxSpeed, deltaTime * 2);
	BoostModifier = FMath::Lerp(BoostModifier, BoostSpeed / MaxSpeed, deltaTime * 2);
	int cameraVertOffset = 450;
	int cameraForwardOffset = 1000 + SpeedModifier * 100 + BoostModifier * 1000;
	float cameraPitch = -5.0f;
	float cameraLerpSpeed = 10.f;
	float cameraVerticalNoise = 0;
	if (bGrounded) {
		float norm = Speed / MaxSpeed * 0.05f + BoostSpeed / MaxSpeed * 1.f;
		cameraVerticalNoise = FMath::RandRange(-1, 1) * 10.f * norm;
		//cameraVertOffset = 500;
		//cameraForwardOffset = 1000;
		FVector normalsSum = FVector::ZeroVector;
		bool bAtLeastOneHit = false;
		FHitResult* hit = new FHitResult();
		FCollisionQueryParams params = FCollisionQueryParams();
		int forwardRaysCount = 4;
		int forwardRaysOffset = 1000; // forwardRaysCount* forwardStartOffset / 2;
		int forwardStartOffset = 300;
		int sideRaysCount = 4;
		int sideRaysOffset = 200;
		int sideStartOffset = (sideRaysCount - 1) * sideRaysOffset / 2;
		int upOffset = 2000;
		int raysLength = 4000;
		for (int i = 0; i < forwardRaysCount; i++) {
			for (int j = 0; j < sideRaysCount; j++) {
				FVector start = ArrowComponent->GetComponentLocation() + GetActorUpVector() * upOffset
					+ ArrowComponent->GetForwardVector() * forwardStartOffset + ArrowComponent->GetForwardVector() * forwardRaysOffset * i
					- ArrowComponent->GetRightVector() * sideStartOffset + ArrowComponent->GetRightVector() * sideRaysOffset * j;
				FVector end = start - SurfaceNormal * raysLength;
				if (GetWorld()->LineTraceSingleByChannel(*hit, start, end, ECollisionChannel::ECC_WorldDynamic)) {
					bAtLeastOneHit = true;
					normalsSum += hit->Normal;
					//DrawDebugLine(GetWorld(), start, end, FColor(0, 255, 0), false, 0.01f, 0, 10);
				}
				else {
					//DrawDebugLine(GetWorld(), start, end, FColor(255, 0, 0), false, 0.01f, 0, 10);
				}
			}
		}
		if (bAtLeastOneHit) {
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + normalsSum * 1000, FColor::Orange, false, 0.01f, 0, 15);
			desiredZ = normalsSum.GetSafeNormal();
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + desiredZ * 1000, FColor::Purple, false, 0.01f, 0, 30);
		}
	}
	else {
		cameraLerpSpeed = 2.f;
		cameraVertOffset = 1000;
		cameraForwardOffset = 1500 + SpeedModifier * 100 + BoostModifier * 1000;
		cameraPitch = -20;
		FVector normalsSum = FVector::ZeroVector;
		bool bAtLeastOneHit = false;
		FHitResult* hit = new FHitResult();
		FCollisionQueryParams params = FCollisionQueryParams();
		int forwardRaysCount = 4;
		int forwardRaysOffset = 800; // forwardRaysCount* forwardStartOffset / 2;
		int forwardStartOffset = (forwardRaysCount - 1) * forwardRaysOffset / 2;
		int sideRaysCount = 4;
		int sideRaysOffset = 800;
		int sideStartOffset = (sideRaysCount - 1)* sideRaysOffset / 2;
		int upOffset = 2000;
		int raysLength = FVector::Distance(ClosestSplinePoint, ArrowComponent->GetComponentLocation());
		for (int i = 0; i < forwardRaysCount; i++) {
			for (int j = 0; j < sideRaysCount; j++) {
				FVector start = ArrowComponent->GetComponentLocation()
					- ArrowComponent->GetForwardVector() * forwardStartOffset + ArrowComponent->GetForwardVector() * forwardRaysOffset * i
					- ArrowComponent->GetRightVector() * sideStartOffset + ArrowComponent->GetRightVector() * sideRaysOffset * j;
				FVector end = start + (ClosestSplinePoint - ArrowComponent->GetComponentLocation()).GetSafeNormal() * raysLength;
				if (GetWorld()->LineTraceSingleByChannel(*hit, start, end, ECollisionChannel::ECC_WorldDynamic)) {
					bAtLeastOneHit = true;
					normalsSum += hit->Normal;
					//DrawDebugLine(GetWorld(), start, end, FColor(0, 255, 0), false, 0.01f, 0, 10);
				}
				else {
					//DrawDebugLine(GetWorld(), start, end, FColor(255, 0, 0), false, 0.01f, 0, 10);
				}
			}
		}
		if (bAtLeastOneHit) {
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + normalsSum * 1000, FColor::Orange, false, 0.01f, 0, 15);
			desiredZ = normalsSum.GetSafeNormal();
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + desiredZ * 1000, FColor::Purple, false, 0.01f, 0, 30);
		}
	}
	ArrowComponent->SetWorldRotation(FRotationMatrix::MakeFromZX(desiredZ, MachineDirection).Rotator());
	ArrowComponentDos->SetWorldRotation(VisibleRotation);
	float tempPitch = 0;
	if (!bGrounded) {
		tempPitch = -MovementInput.Y * 35.f;
	}
	FRotator tempRotation = FMath::Lerp(ArrowComponentDos->GetRelativeRotation(), ArrowComponent->GetRelativeRotation()
		+ FRotator(tempPitch, 0, MovementInput.X * 10.f), deltaTime * cameraLerpSpeed);
	if (MagnetState == MagnetState::DRIFT) {
		tempRotation.Yaw += VirtualDesiredDriftYaw * 0.5f;
	}
	VisibleComponent->SetRelativeRotation(tempRotation);
	VisibleRotation = VisibleComponent->GetComponentRotation();

	float shake = 0;
	if (bShaking) {
		shake = UpdateShake(deltaTime);
	}

	ArrowComponentDos->SetWorldRotation(CameraRotation);
	FRotator tempCamRotation = FMath::Lerp(ArrowComponentDos->GetRelativeRotation(), ArrowComponent->GetRelativeRotation()
		+ FRotator(cameraPitch, 0, 0), deltaTime * cameraLerpSpeed);
	//float cameraHorizontalNoise = FMath::RandRange(-100, 100) * norm;
	CameraLocation = FMath::Lerp(CameraLocation, ArrowComponentDos->GetUpVector() * (cameraVertOffset)
		//+ ArrowComponentDos->GetRightVector() * cameraHorizontalNoise
		- ArrowComponent->GetForwardVector() * cameraForwardOffset, deltaTime * cameraLerpSpeed);
	CameraComponent->SetWorldLocation(CameraLocation + GetActorLocation() + GetActorUpVector() * (cameraVerticalNoise + shake));
	CameraComponent->SetRelativeRotation(tempCamRotation);
	CameraRotation = CameraComponent->GetComponentRotation();
	CameraComponent->FieldOfView = 90 + 3 * SpeedModifier + 50 * BoostModifier;

	UIParallax = FMath::Lerp(UIParallax,
		FVector(-tempCamRotation.Yaw * 0.075f, (tempCamRotation.Pitch - cameraPitch) * 0.15f, FMath::Clamp((Speed + BoostSpeed * 5) / MaxSpeed - 1, -1, 5)),
		deltaTime * 4.f) + FVector::RightVector * (cameraVerticalNoise * 0.04f + shake * 0.005f);
}

void APlayerMachine::StartShake(float inAmplitude) {
	ShakeAmplitude = inAmplitude;
	ShakeTime = FMath::RandRange(0.f, PI / 2.f);
	bShaking = true;
}

float APlayerMachine::UpdateShake(float deltaTime) {
	ShakeTime += deltaTime;
	float frequency = 10.f;
	float shakeDecrementSpeed = 6000.f;
	float shake = FMath::PerlinNoise1D((ShakeTime + 1) * frequency) * ShakeAmplitude;
	ShakeAmplitude -= shakeDecrementSpeed * deltaTime;
	if (ShakeAmplitude <= 0) {
		bShaking = false;
	}
	return shake;
}

void APlayerMachine::MoveRight(float AxisValue)
{
	if (bCanRace) {
		float speedClamp = FMath::Clamp(Speed, 0.f, 100.f) / 100.f;
		MovementInput.X = FMath::Clamp(AxisValue, -1.0f, 1.0f) * speedClamp;
	}
}
void APlayerMachine::GripRight(float AxisValue)
{
	//RightGrip = AxisValue * FMath::Abs(MovementInput.X) * GripInputMultiplier;
	RightGrip = AxisValue * GripInputMultiplier;
	if (bDoubleGripOnDrift) {
		RightGrip *= 2;
	}
}
void APlayerMachine::GripLeft(float AxisValue)
{
	//LeftGrip = AxisValue * FMath::Abs(MovementInput.X) * GripInputMultiplier;
	LeftGrip = AxisValue * GripInputMultiplier;
	if (bDoubleGripOnDrift) {
		LeftGrip *= 2;
	}
}

void APlayerMachine::MoveForward(float AxisValue)
{
	if (bCanRace) {
		MovementInput.Y = FMath::Clamp(AxisValue, -1.0f, 1.0f);
	}
}

void APlayerMachine::Accelerate()
{
	bAccelerating = true;
}

void APlayerMachine::Deccelerate()
{
	bAccelerating = false;
}

void APlayerMachine::Brake() {
	bBraking = true;
}

void APlayerMachine::LiftBrake() {
	bBraking = false;
}

void APlayerMachine::ExpelBoost() {
	if (bCanRace && bAccelerating && BoostRemainingTime == 0) {
		BoostRemainingTime = BoostDurationTime;
	}
}

void APlayerMachine::Pause() {
	TrackManager->Pause();
}