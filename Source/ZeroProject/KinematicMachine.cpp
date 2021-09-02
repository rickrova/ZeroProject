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
	MachineLastPosition = KinematicComponent->GetComponentLocation();
}

void AKinematicMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!MovementInput.IsZero())
	{
		FRotator deltaRotation = FRotator::ZeroRotator;
		deltaRotation.Yaw = MovementInput.X * DeltaTime * Steering;

		if (VerticalSpeed > 0) {
			//deltaRotation.Pitch = -MovementInput.Y * DeltaTime * Steering;
		}

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
	FVector deltaLocation = ArrowComponent->GetForwardVector() * Speed * DeltaTime;

	FHitResult* hit = new FHitResult();
	KinematicComponent->MoveComponent(deltaLocation, FQuat::Identity, true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
	if (hit->bBlockingHit) {
		Speed -= hit->PenetrationDepth;
		FVector deflectedLocation = hit->ImpactPoint + hit->Normal * 12;
		KinematicComponent->SetWorldLocation(deflectedLocation);
	}
	Raycast(DeltaTime);

	FRotator desiredRotation = ArrowComponent->GetComponentRotation();
	FRotator deltaRotation = desiredRotation - CameraContainerComponent->GetComponentRotation();
	FRotator epsilonRotation = deltaRotation.GetNormalized() * DeltaTime * 10;
	CameraContainerComponent->SetWorldRotation(CameraContainerComponent->GetComponentRotation() + epsilonRotation);

	FRotator rotationWithRoll = CameraContainerComponent->GetComponentRotation();
	rotationWithRoll.Roll += (1 - 1 / (1 + FMath::Abs(MovementInput.X) * 4)) * FMath::Sign(MovementInput.X) * 20;
	if (VerticalSpeed > 0) {
		rotationWithRoll.Pitch += (1 - 1 / (1 + FMath::Abs(MovementInput.Y) * 4)) * -FMath::Sign(MovementInput.Y) * 20;
	}
	VisibleComponent->SetWorldRotation(rotationWithRoll);  //FMath::Lerp(VisibleComponent->GetComponentRotation(), rotationWithRoll, DeltaTime * 10));
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
}

void AKinematicMachine::Raycast(float deltaTime)
{
	FHitResult* hit = new FHitResult();
	FCollisionQueryParams params = FCollisionQueryParams();
	params.AddIgnoredActor(this);

	FVector start = ArrowComponent->GetComponentLocation();
	FVector end = start + GravityDirection * RayDistance;
	DrawDebugLine(GetWorld(), start, end, FColor::Orange, false);
	if (GetWorld()->LineTraceSingleByChannel(*hit, start, end, ECC_GameTraceChannel1, params)) {
		LastNormal = hit->Normal;
		GravityDirection = -LastNormal;
		if (VerticalSpeed == 0) {
			FRotator desiredRoation = FRotationMatrix::MakeFromZX(LastNormal, ArrowComponent->GetForwardVector()).Rotator();
			ArrowComponent->SetWorldRotation(desiredRoation);
		}
	}
	end = start + GravityDirection * RayDistance;
	if (GetWorld()->LineTraceSingleByChannel(*hit, start, end, ECC_GameTraceChannel1, params)) {
		if (hit->Distance > 20) {
			VerticalSpeed += Gravity * deltaTime * deltaTime;
			FVector desiredLocation = start + GravityDirection * VerticalSpeed;
			if (VerticalSpeed + 20 < hit->Distance) {
				KinematicComponent->SetWorldLocation(desiredLocation);
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
	}
	else {
		VerticalSpeed += Gravity * deltaTime * deltaTime;
		FVector desiredLocation = start + GravityDirection * VerticalSpeed;
		KinematicComponent->SetWorldLocation(desiredLocation);
	}
}

void AKinematicMachine::CheckDesiredLocation(FVector desiredLocation) {
	FHitResult* hit = new FHitResult();
	FVector machineCurrentLocation = KinematicComponent->GetComponentLocation();
	FCollisionQueryParams params = FCollisionQueryParams();
	params.AddIgnoredActor(this);

	DrawDebugLine(GetWorld(), machineCurrentLocation, desiredLocation, FColor::Green, false);
	if (GetWorld()->LineTraceSingleByChannel(*hit, machineCurrentLocation, desiredLocation, ECC_WorldDynamic, params)) {
		if (hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic) {
			GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("  WALL")));
			FVector reflectedDirection = (machineCurrentLocation - hit->ImpactPoint).MirrorByVector(hit->ImpactNormal);
			float deltaSize = (machineCurrentLocation - desiredLocation).Size() - hit->Distance;
			FVector deflectedLocation = hit->ImpactPoint + reflectedDirection * deltaSize;
			//KinematicComponent->MoveComponent(deltaLocation, FRotator::ZeroRotator, false, 0, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::ResetPhysics);
		}
		else {
			GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("  FLOOR")));
		}
	}
}

void AKinematicMachine::MoveRight(float AxisValue)
{
	MovementInput.X = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void AKinematicMachine::DriftRight(float AxisValue)
{
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f + AxisValue, 1.0f);
	if (MovementInput.X > 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, AxisValue);
	}
}
void AKinematicMachine::DriftLeft(float AxisValue)
{
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f, 1.0f - AxisValue);
	if (MovementInput.X < 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, AxisValue);
	}
}

void AKinematicMachine::MoveForward(float AxisValue)
{
	MovementInput.Y = FMath::Clamp(AxisValue, -1.0f, 1.0f);
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

