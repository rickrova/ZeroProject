// Fill out your copyright notice in the Description page of Project Settings.
#include "KinematicMachine.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"

AKinematicMachine::AKinematicMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	KinematicComponent = CreateDefaultSubobject<USphereComponent>(TEXT("KinematicComponent"));
	VisibleComponent = CreateDefaultSubobject <UStaticMeshComponent>(TEXT("VisibleComponent"));
	ArrowComponent = CreateDefaultSubobject <UArrowComponent>(TEXT("ArrowComponent"));
	CameraContainerComponent = CreateDefaultSubobject <UArrowComponent>(TEXT("CameraContinerComponent"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));

	KinematicComponent->SetupAttachment(RootComponent);
	VisibleComponent->SetupAttachment(KinematicComponent);
	ArrowComponent->SetupAttachment(KinematicComponent);
	CameraContainerComponent->SetupAttachment(KinematicComponent);
	SpringArmComponent->SetupAttachment(CameraContainerComponent);
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);

	SpringArmComponent->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 50.0f), FRotator(-15.0f, 0.0f, 0.0f));
	SpringArmComponent->TargetArmLength = 400.f;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 3.0f;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AKinematicMachine::BeginPlay()
{
	Super::BeginPlay();
	KinematicComponent->SetWorldRotation(FQuat::Identity);
	ArrowComponent->SetWorldRotation(RootComponent->GetComponentRotation());
	VisibleComponent->SetWorldRotation(RootComponent->GetComponentRotation());
	CameraContainerComponent->SetWorldRotation(RootComponent->GetComponentRotation());
	AirYaw = RootComponent->GetComponentRotation().Yaw;
	GravityDirection = -RootComponent->GetUpVector();

	TimerDel.BindUFunction(this, FName("ExitDrift"));
	DriftDelegate.BindUFunction(this, FName("ResetDrift"));
	ExitDriftDelegate.BindUFunction(this, FName("ResetExitDrift"));
}

void AKinematicMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Ensure the fuze timer is cleared by using the timer handle
	//GetWorld()->GetTimerManager().ClearTimer(FuzeTimerHandle);

	// Alternatively you can clear ALL timers that belong to this (Actor) instance.
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AKinematicMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator deltaRotation = FRotator::ZeroRotator;
	if (!MovementInput.IsZero() && bGrounded && !bDrifting)
	{
		deltaRotation.Yaw = MovementInput.X * DeltaTime * Steering;
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Delta yaw: %f"), deltaRotation.Yaw));
		if (deltaRotation.Yaw > DriftThereshold && bCanDrift && Speed > MaxSpeed * 0.9f) {
			bDrifting = true;
			GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
			DriftingOffset = 0.0f;
		}else if (deltaRotation.Yaw < -DriftThereshold && bCanDrift && Speed > MaxSpeed * 0.9f) {
			bDrifting = true;
			GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
			DriftingOffset = 0.0f;
		}
	}
	else if (bGrounded && bDrifting) {
		deltaRotation.Yaw = (1 - FVector::DotProduct( CameraContainerComponent->GetForwardVector(), VisibleComponent->GetForwardVector()))
			* DeltaTime * 100.f * FMath::Sign(FVector::DotProduct(CameraContainerComponent->GetRightVector(), VisibleComponent->GetForwardVector()));
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Delta yaw: %f"), deltaRotation.Yaw));
		DriftYaw -= deltaRotation.Yaw/5;
		//DriftYaw -= 10.f * FMath::Sign(deltaRotation.Yaw) * DeltaTime;

		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("pending drift %s"), bPendingDrift ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("can exit drift %s"), bCanExitDrift ? TEXT("true") : TEXT("false")));
		if (FMath::Abs(deltaRotation.Yaw) < 0.05f && !bPendingDrift && bCanExitDrift) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Timer to exit drift")));
			GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, 0.01f, false, 0.01f);
			bPendingDrift = true;
		}
		else if(FMath::Abs(deltaRotation.Yaw) >= 0.05f && bPendingDrift){
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Timer cancelled")));
			bPendingDrift = false;
			GetWorldTimerManager().ClearTimer(TimerHandle);
		}
	}
	ArrowComponent->AddLocalRotation(deltaRotation);

	if (bAccelerating)
	{
		Speed += AccelerationRate * DeltaTime * DeltaTime;
	}
	else {
		Speed -= DeccelerationRate * DeltaTime * DeltaTime;
	}

	Speed = FMath::Clamp(Speed, 0.f, MaxSpeed);

	FVector deltaLocation = ArrowComponent->GetForwardVector() * (Speed + SpeedModifier);

	FHitResult* hit = new FHitResult();
	KinematicComponent->MoveComponent(deltaLocation, FQuat::Identity, true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
	if (hit->bBlockingHit && hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic) {
		float impactAngle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(ArrowComponent->GetForwardVector(), hit->Normal)));
        float decimationFactor = 1 - (impactAngle - 90) / 90;
        Speed *= decimationFactor;
        SpeedModifier *= decimationFactor;
		FVector deflectedLocation = hit->ImpactPoint + hit->Normal * 12;
		KinematicComponent->SetWorldLocation(deflectedLocation);
		FVector selectedNormal = -hit->Normal;
		if (FVector::DotProduct(hit->Normal, ArrowComponent->GetRightVector()) > 0) {
			selectedNormal *= -1;
		}
		FRotator deflectedRotation = FRotationMatrix::MakeFromZY(ArrowComponent->GetUpVector(), selectedNormal).Rotator();
		ArrowComponent->SetWorldRotation(deflectedRotation);
		//VisibleComponent->SetWorldRotation(deflectedRotation);
	}
	Raycast(DeltaTime);
	float speed = FVector::Distance(KinematicComponent->GetComponentLocation(), LastMachineLocation) / DeltaTime;//cm / seg
	speed *= 60; //cm / min
	speed *= 60; //cm / h
	speed /= 100; // m / h
	speed /= 1000; // km / k
	speed *= 10; //scale adjustments
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Speed: %f"), speed));
	CameraComponent->FieldOfView = FMath::Clamp(FMath::Lerp(CameraComponent->FieldOfView, speed/22 + 60, DeltaTime * 5), 100.f, 180.f);
	LastMachineLocation = KinematicComponent->GetComponentLocation();
    SpeedModifier -= DeltaTime * 10;
    SpeedModifier = FMath::Clamp(SpeedModifier, 0.f, BoostSpeed);
}

void AKinematicMachine::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis("MoveX", this, &AKinematicMachine::MoveRight);
	InputComponent->BindAxis("MoveY", this, &AKinematicMachine::MoveForward);
	InputComponent->BindAxis("GripRight", this, &AKinematicMachine::GripRight);
	InputComponent->BindAxis("GripLeft", this, &AKinematicMachine::GripLeft);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Pressed, this, &AKinematicMachine::Accelerate);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Released, this, &AKinematicMachine::Deccelerate);
	InputComponent->BindAction("Brake", EInputEvent::IE_Pressed, this, &AKinematicMachine::Brake);
	InputComponent->BindAction("Brake", EInputEvent::IE_Released, this, &AKinematicMachine::LiftBrake);
    InputComponent->BindAction("Boost", EInputEvent::IE_Pressed, this, &AKinematicMachine::Boost);
	InputComponent->BindAction("DriftLeft", EInputEvent::IE_Pressed, this, &AKinematicMachine::DriftLeft);
	InputComponent->BindAction("DriftRight", EInputEvent::IE_Pressed, this, &AKinematicMachine::DriftRight);
}

void AKinematicMachine::Raycast(float deltaTime)
{
	FRotator desiredRotation = CameraContainerComponent->GetComponentRotation();
	if (bGrounded) {
		FVector normalsSum = FVector::ZeroVector;
		bool bAtLeastOneHit = false;
		for (int i = 0; i < Raycount; i++) {

			FHitResult* hit = new FHitResult();
			FCollisionQueryParams params = FCollisionQueryParams();
			params.AddIgnoredActor(this);
			FVector start = ArrowComponent->GetComponentLocation() + ArrowComponent->GetUpVector() * RaySetVerticalOfset - ArrowComponent->GetForwardVector() * RaySetOffset + ArrowComponent->GetForwardVector() * RaysOffset * i;
			FVector end = start + GravityDirection * RaySetDistance;
			//DrawDebugLine(GetWorld(), start, end, FColor::Orange, false);
			if (GetWorld()->LineTraceSingleByChannel(*hit, start, end, ECC_GameTraceChannel1, params)) {
				bAtLeastOneHit = true;
				normalsSum += hit->Normal;
			}
		}
		if (bAtLeastOneHit) {
			normalsSum.Normalize();
			GravityDirection = -normalsSum;
			desiredRotation = FRotationMatrix::MakeFromZX(normalsSum, ArrowComponent->GetForwardVector()).Rotator();
		}
	}

	FRotator deltaRotation = desiredRotation - CameraContainerComponent->GetComponentRotation();
	FRotator epsilonRotation = deltaRotation.GetNormalized() * deltaTime * 10;
	CameraContainerComponent->SetWorldRotation(CameraContainerComponent->GetComponentRotation() + epsilonRotation);

	FRotator rotationWithRoll = CameraContainerComponent->GetComponentRotation();
	if (bGrounded) {
		if (!bDrifting) {
			rotationWithRoll.Roll += (1 - 1 / (1 + FMath::Abs(MovementInput.X) * 4)) * FMath::Sign(MovementInput.X) * 20 * (1 + RightDrift + LeftDrift);
		}
		AirYaw = rotationWithRoll.Yaw;
	}
	else {
		rotationWithRoll.Pitch += (1 - 1 / (1 + FMath::Abs(MovementInput.Y) * 4)) * -FMath::Sign(MovementInput.Y) * 45;
		AirYaw += (MovementInput.X + RightDrift - LeftDrift) * deltaTime * Steering;
		rotationWithRoll.Yaw = AirYaw;
	}
	VisibleComponent->SetWorldRotation(FMath::Lerp(VisibleComponent->GetComponentRotation(), rotationWithRoll, deltaTime * 10));
	if (bGrounded && bDrifting) {
		DriftYaw += (MovementInput.X + (RightDrift - LeftDrift) + DriftingOffset) * deltaTime * 20;
		DriftYaw = FMath::Clamp(DriftYaw, -15.f, 15.f);
		FRotator deltaRotator = FRotator::ZeroRotator;
		deltaRotator.Yaw = DriftYaw;
		VisibleComponent->AddLocalRotation(deltaRotator);
	}

	FHitResult* hit = new FHitResult();
	FCollisionQueryParams params = FCollisionQueryParams();
	params.AddIgnoredActor(this);
	FVector start = ArrowComponent->GetComponentLocation();
	FVector end = start + GravityDirection * RayDistance;
	if (GetWorld()->LineTraceSingleByChannel(*hit, start, end, ECC_GameTraceChannel1, params)) {
		if (bGrounded) {
			desiredRotation = FRotationMatrix::MakeFromZX(hit->Normal, ArrowComponent->GetForwardVector()).Rotator();
			ArrowComponent->SetWorldRotation(desiredRotation);
		}
		if (hit->Distance > 20) {
			VerticalSpeed += Gravity * deltaTime * deltaTime;
			if (VerticalSpeed + 20 < hit->Distance) {
				KinematicComponent->MoveComponent(GravityDirection * VerticalSpeed * VerticalSpeedModifier, FQuat::Identity, true);
			}
			else {
				VerticalSpeed = 0;
				KinematicComponent->SetWorldLocation(hit->ImpactPoint - GravityDirection * 20);
			}
		}
		else {
			VerticalSpeed = 0;
			KinematicComponent->SetWorldLocation(hit->ImpactPoint - GravityDirection * 20);
		}

		if (hit->Distance > 30 && bGrounded) {
			bGrounded = false;
		}
		else if(hit->Distance <= 30 && !bGrounded){
			bGrounded = true;
            float tempYaw = VisibleComponent->GetComponentRotation().Yaw - ArrowComponent->GetComponentRotation().Yaw;
			VisibleComponent->SetWorldRotation(ArrowComponent->GetComponentRotation());
            FRotator tempRotator = FRotator::ZeroRotator;
            tempRotator.Yaw = tempYaw;
            VisibleComponent->AddLocalRotation(tempRotator);
            float deltaYaw = ArrowComponent->GetComponentRotation().Yaw - tempYaw;
            if(deltaYaw > 10 && Speed > MaxSpeed * 0.9f && bCanDrift){
                bDrifting = true;
				GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
				DriftingOffset = 0.0f;
			}
			else if (deltaYaw < -10 && Speed > MaxSpeed * 0.9f && bCanDrift) {
				bDrifting = true;
				GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
				DriftingOffset = 0.0f;
			}
		}
	}
	else {
		if (bGrounded) {
			bGrounded = false;
		}
		VerticalSpeed += Gravity * deltaTime * deltaTime;
		KinematicComponent->MoveComponent(GravityDirection * VerticalSpeed * VerticalSpeedModifier, FQuat::Identity, true);
	}
}

void AKinematicMachine::MoveRight(float AxisValue)
{
	MovementInput.X = FMath::Clamp(AxisValue, -1.0f, 1.0f);
	RawMovementInput.X = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void AKinematicMachine::GripRight(float AxisValue)
{
	RightDrift = AxisValue;
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f + RightDrift, 1.0f);
	if (MovementInput.X > 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, RightDrift);
	}
}
void AKinematicMachine::GripLeft(float AxisValue)
{
	LeftDrift = AxisValue;
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f, 1.0f - LeftDrift);
	if (MovementInput.X < 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, LeftDrift);
	}
}
void AKinematicMachine::DriftLeft()
{
	if (!bDrifting && Speed > MaxSpeed * 0.9f && bCanDrift && MovementInput.X < 0) {
		bDrifting = true;
		bCanExitDrift = false;
		GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
	}
}
void AKinematicMachine::DriftRight()
{
	if (!bDrifting && Speed > MaxSpeed * 0.9f && bCanDrift && MovementInput.X > 0) {
		bDrifting = true;
		bCanExitDrift = false;
		GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
	}
}

void AKinematicMachine::MoveForward(float AxisValue)
{
	MovementInput.Y = FMath::Clamp(AxisValue, -1.0f, 1.0f);
	if (bGrounded) {
		VerticalSpeedModifier = 1;
	}
	else {
		VerticalSpeedModifier += MovementInput.Y * GetWorld()->GetDeltaSeconds() * 10;
        VerticalSpeedModifier = FMath::Clamp(VerticalSpeedModifier, 0.15f, 6.f);
	}
}

void AKinematicMachine::Accelerate()
{
	if (!bBraking) {
		bAccelerating = true;
	}
	else {
		bPendingAcceleration = true;
	}
}

void AKinematicMachine::Deccelerate()
{
	if (!bBraking) {
		bAccelerating = false;
	}
	else {
		bPendingAcceleration = false;
	}
}

void AKinematicMachine::Brake() {
	if (bAccelerating) {
		bPendingAcceleration = true;
	}
	else {
		bPendingAcceleration = false;
	}
	DeccelerationRate *= 4;
	bAccelerating = false;
	bBraking = true;
}

void AKinematicMachine::LiftBrake() {
	DeccelerationRate /= 4;
	bBraking = false;
	if (bPendingAcceleration) {
		Accelerate();
	}
	else {
		Deccelerate();
	}
}

void AKinematicMachine::Boost(){
    SpeedModifier = BoostSpeed;
}

void AKinematicMachine::ExitDrift() {
	if (bCanExitDrift) {
		bDrifting = false;
		bPendingDrift = false;
		DriftingOffset = 0.f;
		DriftYaw = 0.f;
		bCanDrift = false;

		GetWorldTimerManager().SetTimer(TimerHandle, DriftDelegate, 1.f, false, 1.f);
	}
}

void AKinematicMachine::ResetDrift() {
	bCanDrift = true;
}

void AKinematicMachine::ResetExitDrift() {
	bCanExitDrift = true;
}

