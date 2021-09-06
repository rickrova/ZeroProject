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
}

void AKinematicMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!MovementInput.IsZero() && bGrounded)
	{
		FRotator deltaRotation = FRotator::ZeroRotator;
		deltaRotation.Yaw = MovementInput.X * DeltaTime * Steering;

		ArrowComponent->AddLocalRotation(deltaRotation);
	}

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
		Speed *= 1 - (impactAngle - 90) / 90;
		FVector deflectedLocation = hit->ImpactPoint + hit->Normal * 12;
		KinematicComponent->SetWorldLocation(deflectedLocation);
		FVector selectedNormal = -hit->Normal;
		if (FVector::DotProduct(hit->Normal, ArrowComponent->GetRightVector()) > 0) {
			selectedNormal *= -1;
		}
		FRotator deflectedRotation = FRotationMatrix::MakeFromZY(ArrowComponent->GetUpVector(), selectedNormal).Rotator();
		ArrowComponent->SetWorldRotation(deflectedRotation);
		VisibleComponent->SetWorldRotation(deflectedRotation);
	}
	Raycast(DeltaTime);
	float speed = FVector::Distance(KinematicComponent->GetComponentLocation(), LastMachineLocation) / DeltaTime;//cm / seg
	speed *= 60; //cm / min
	speed *= 60; //cm / h
	speed /= 100; // m / h
	speed /= 1000; // km / k
	speed *= 10; //scale adjustments
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Speed: %f", speed));
	CameraComponent->FieldOfView = FMath::Lerp(CameraComponent->FieldOfView, speed/100 + 100, DeltaTime * 5);
	LastMachineLocation = KinematicComponent->GetComponentLocation();
    SpeedModifier -= DeltaTime * 10;
    SpeedModifier = FMath::Clamp(SpeedModifier, 0.f, BoostSpeed);
}

void AKinematicMachine::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis("MoveX", this, &AKinematicMachine::MoveRight);
	InputComponent->BindAxis("MoveY", this, &AKinematicMachine::MoveForward);
	InputComponent->BindAxis("DriftRight", this, &AKinematicMachine::DriftRight);
	InputComponent->BindAxis("DriftLeft", this, &AKinematicMachine::DriftLeft);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Pressed, this, &AKinematicMachine::Accelerate);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Released, this, &AKinematicMachine::Deccelerate);
	InputComponent->BindAction("Brake", EInputEvent::IE_Pressed, this, &AKinematicMachine::Brake);
	InputComponent->BindAction("Brake", EInputEvent::IE_Released, this, &AKinematicMachine::LiftBrake);
    InputComponent->BindAction("Boost", EInputEvent::IE_Pressed, this, &AKinematicMachine::Boost);
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
			DrawDebugLine(GetWorld(), start, end, FColor::Orange, false);
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
		rotationWithRoll.Roll += (1 - 1 / (1 + FMath::Abs(MovementInput.X) * 4)) * FMath::Sign(MovementInput.X) * 20 * (1 + RightDrift + LeftDrift);
		AirYaw = rotationWithRoll.Yaw;
	}
	else {
		rotationWithRoll.Pitch += (1 - 1 / (1 + FMath::Abs(MovementInput.Y) * 4)) * -FMath::Sign(MovementInput.Y) * 45;
		AirYaw += (MovementInput.X + RightDrift - LeftDrift) * deltaTime * Steering;
		rotationWithRoll.Yaw = AirYaw;
	}
	VisibleComponent->SetWorldRotation(FMath::Lerp(VisibleComponent->GetComponentRotation(), rotationWithRoll, deltaTime * 10));

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
			VisibleComponent->SetWorldRotation(ArrowComponent->GetComponentRotation());
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

void AKinematicMachine::DriftRight(float AxisValue)
{
	RightDrift = AxisValue;
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f + RightDrift, 1.0f);
	if (MovementInput.X > 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, RightDrift);
	}
}
void AKinematicMachine::DriftLeft(float AxisValue)
{
	LeftDrift = AxisValue;
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f, 1.0f - LeftDrift);
	if (MovementInput.X < 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, LeftDrift);
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
        VerticalSpeedModifier = FMath::Clamp(VerticalSpeedModifier, 0.25f, 4.f);
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

