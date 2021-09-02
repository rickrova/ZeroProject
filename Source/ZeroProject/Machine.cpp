// Fill out your copyright notice in the Description page of Project Settings.


#include "Machine.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"

// Sets default values
AMachine::AMachine()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	PhysicsComponent = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsComponent"));
	VisibleComponent = CreateDefaultSubobject <UStaticMeshComponent>(TEXT("VisibleComponent"));
	ArrowComponent = CreateDefaultSubobject <UArrowComponent>(TEXT("ArrowComponent"));
	CameraContainerComponent = CreateDefaultSubobject <UArrowComponent>(TEXT("CameraContinerComponent"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));

	PhysicsComponent->SetupAttachment(RootComponent);
	VisibleComponent->SetupAttachment(PhysicsComponent);
	ArrowComponent->SetupAttachment(PhysicsComponent);
	CameraContainerComponent->SetupAttachment(PhysicsComponent);
	SpringArmComponent->SetupAttachment(CameraContainerComponent);
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);

	//Setting Default Variables and Behavior of the SpringArmComponent
	SpringArmComponent->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 50.0f), FRotator(-15.0f, 0.0f, 0.0f));
	SpringArmComponent->TargetArmLength = 400.f;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 3.0f;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void AMachine::BeginPlay()
{
	Super::BeginPlay();
	MachineLastPosition = PhysicsComponent->GetComponentLocation();
}

// Called every frame
void AMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Raycast();

	FVector desiredForce = GravityDirection * GravityForce;

	if (!MovementInput.IsZero())
	{
		FRotator deltaRotation = FRotator::ZeroRotator; 
		deltaRotation.Yaw = MovementInput.X * DeltaTime * Steering;
		ArrowComponent->AddLocalRotation(deltaRotation);
	}

	FVector relativeSurfaceMovement = FVector::VectorPlaneProject(PhysicsComponent->GetPhysicsLinearVelocity(), ArrowComponent->GetUpVector());
	FVector relativeGravityMovement = PhysicsComponent->GetPhysicsLinearVelocity() - relativeSurfaceMovement;
	PhysicsComponent->SetPhysicsLinearVelocity(FMath::Lerp(ArrowComponent->GetForwardVector()
		* FMath::Clamp(relativeSurfaceMovement.Size(), 0.0f, MaxSpeed) - ArrowComponent->GetUpVector() * relativeGravityMovement.Size(),
		PhysicsComponent->GetPhysicsLinearVelocity(), PhysicsFactor), false);

	if (bAccelerating)
	{
		FVector desiredDirection = ArrowComponent->GetForwardVector();
		desiredForce += desiredDirection * Acceleration;
	}
	PhysicsComponent->AddForce(desiredForce);
	MachineLastPosition = PhysicsComponent->GetComponentLocation();

	FRotator desiredRotation = ArrowComponent->GetComponentRotation();
	FRotator deltaRotation = desiredRotation - CameraContainerComponent->GetComponentRotation();
	FRotator epsilonRotation = deltaRotation.GetNormalized() * DeltaTime * 10;

	CameraContainerComponent->SetWorldRotation(CameraContainerComponent->GetComponentRotation() + epsilonRotation);

	FRotator rotationWithRoll = CameraContainerComponent->GetComponentRotation();
	rotationWithRoll.Roll += FMath::Loge( 1 + FMath::Abs(MovementInput.X) * 15) * FMath::Sign(MovementInput.X) * 10;
	VisibleComponent->SetWorldRotation(FMath::Lerp(VisibleComponent->GetComponentRotation(), rotationWithRoll, DeltaTime * 10));

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Speed: %f"), PhysicsComponent->GetPhysicsLinearVelocity().Size()));	
}

// Called to bind functionality to input
void AMachine::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Respond every frame to the values of our two movement axes, "MoveX" and "MoveY".
	InputComponent->BindAxis("MoveX", this, &AMachine::MoveRight);
	InputComponent->BindAxis("MoveY", this, &AMachine::MoveForward);
	InputComponent->BindAxis("DriftRight", this, &AMachine::DriftRight);
	InputComponent->BindAxis("DriftLeft", this, &AMachine::DriftLeft);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Pressed, this, &AMachine::Accelerate);
	InputComponent->BindAction("Accelerate", EInputEvent::IE_Released, this, &AMachine::Deccelerate);
	InputComponent->BindAction("Brake", EInputEvent::IE_Pressed, this, &AMachine::Brake);
	InputComponent->BindAction("Brake", EInputEvent::IE_Released, this, &AMachine::LiftBrake);
}

void AMachine::Raycast()
{
	FHitResult* hit = new FHitResult();
	FVector machineCurrentPosition = PhysicsComponent->GetComponentLocation();
	FCollisionQueryParams params = FCollisionQueryParams();
	params.AddIgnoredActor(this);

	DrawDebugLine(GetWorld(), MachineLastPosition, machineCurrentPosition, FColor::Green, false);
	if (GetWorld()->LineTraceSingleByChannel(*hit, MachineLastPosition, machineCurrentPosition, ECC_WorldDynamic, params)) {
		FVector reflectedDirection = (machineCurrentPosition - hit->ImpactPoint).MirrorByVector(hit->ImpactNormal);
		FVector desiredLocation = hit->ImpactPoint + reflectedDirection;
		FVector deltaLocation = desiredLocation - machineCurrentPosition;
		/*GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("   CurrentPosition: %s"), *machineCurrentPosition.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("   ImpactPoint: %s"), *hit->ImpactPoint.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("   DesiredLocation: %s"), *desiredLocation.ToString()));*/
		if(hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic) {
			GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("  WALL")));
			PhysicsComponent->MoveComponent(deltaLocation, FRotator::ZeroRotator, false, 0, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::ResetPhysics);
			PhysicsComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
		}else {
			GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("  FLOOR")));
			PhysicsComponent->SetPhysicsLinearVelocity(reflectedDirection.GetSafeNormal() * PhysicsComponent->GetPhysicsLinearVelocity().Size());
			PhysicsComponent->MoveComponent(deltaLocation, FRotator::ZeroRotator, false, 0, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
			GravityDirection = -hit->Normal;
		}
		GEngine->AddOnScreenDebugMessage(-1, 2000.f, FColor::Yellow, FString::Printf(TEXT("Crash:")));
	}

	FVector direction = GravityDirection;
	FVector start = ArrowComponent->GetComponentLocation();
	FVector end = start + direction * RayDistance;
	DrawDebugLine(GetWorld(), start, end, FColor::Orange, false);
	if (GetWorld()->LineTraceSingleByChannel(*hit, start, end, ECC_GameTraceChannel1, params)) {
		LastContact = hit->ImpactPoint;
		LastNormal = hit->Normal;
		GravityDirection = -LastNormal;
		FRotator desiredRoation = FRotationMatrix::MakeFromZX(LastNormal, ArrowComponent->GetForwardVector()).Rotator();
		ArrowComponent->SetWorldRotation(desiredRoation);
	}
	else {
		//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("on air"));
	}
}

void AMachine::MoveRight(float AxisValue)
{
	// Move at 100 units per second forward or backward
	MovementInput.X = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void AMachine::DriftRight(float AxisValue)
{
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f + AxisValue, 1.0f);
	if (MovementInput.X > 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, AxisValue);
		PhysicsFactor = FMath::Lerp(MinPhysicsFactor, 1.f, AxisValue);
	}
}
void AMachine::DriftLeft(float AxisValue)
{
	MovementInput.X = FMath::Clamp(MovementInput.X, -1.0f, 1.0f - AxisValue);
	if (MovementInput.X < 0)
	{
		Steering = FMath::Lerp(MinSteering, MaxSteering, AxisValue);
		PhysicsFactor = FMath::Lerp(MinPhysicsFactor, 1.f, AxisValue);
	}
}

void AMachine::MoveForward(float AxisValue)
{
	// Move at 100 units per second right or left
	MovementInput.Y = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void AMachine::Accelerate()
{
	if (!bBraking) {
		PhysicsComponent->SetPhysMaterialOverride(CeroFrictionMaterial->GetPhysicalMaterial());
		bAccelerating = true;
	}
	else {
		bPendingAcceleration = true;
	}
}

void AMachine::Deccelerate()
{
	if (!bBraking) {
		PhysicsComponent->SetPhysMaterialOverride(SomeFrictionMaterial->GetPhysicalMaterial());
		bAccelerating = false;
	}
	else {
		bPendingAcceleration = false;
	}
}

void AMachine::Brake() {
	if (bAccelerating) {
		bPendingAcceleration = true;
	}
	else {
		bPendingAcceleration = false;
	}
	PhysicsComponent->SetPhysMaterialOverride(FullFrictionMaterial->GetPhysicalMaterial());
	bAccelerating = false;
	bBraking = true;
}

void AMachine::LiftBrake() {
	bBraking = false;
	if (bPendingAcceleration) {
		Accelerate();
	}
	else {
		Deccelerate();
	}
}

