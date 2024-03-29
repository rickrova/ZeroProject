// Fill out your copyright notice in the Description page of Project Settings.
#include "KinematicMachine.h"
#include "AIMachine.h"
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
    
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AKinematicMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator deltaRotation = FRotator::ZeroRotator;
	if (!MovementInput.IsZero() && bGrounded && !bDrifting)
	{
		deltaRotation.Yaw = MovementInput.X * DeltaTime * Steering;
		if ((Speed + SpeedModifier) > MaxSpeed * 0.9f && bCanDrift && FMath::Abs(MovementInput.X + RightDrift - LeftDrift) > DriftThereshold) {
			bDrifting = true;
			bCanExitDrift = false;
			GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
		}
	}
	else if (bGrounded && bDrifting) {
		deltaRotation.Yaw = (1 - FVector::DotProduct( CameraContainerComponent->GetForwardVector(), VisibleComponent->GetForwardVector()))
			* DeltaTime * 100.f * FMath::Sign(FVector::DotProduct(CameraContainerComponent->GetRightVector(), VisibleComponent->GetForwardVector()));
		DriftYaw -= deltaRotation.Yaw/5;

		if (FMath::Abs(deltaRotation.Yaw) < 0.05f && !bPendingDrift && bCanExitDrift) {
			GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, 0.01f, false, 0.01f);
			bPendingDrift = true;
		}
		else if(FMath::Abs(deltaRotation.Yaw) >= 0.05f && bPendingDrift){
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

	DeltaLocation = ArrowComponent->GetForwardVector() * (Speed + SpeedModifier) * DeltaTime * 10 + HitDelta * DeltaTime;

    HitDelta = FMath::Lerp(HitDelta, FVector::ZeroVector, DeltaTime * HitDecceleration);
    
	FHitResult* hit = new FHitResult();
	KinematicComponent->MoveComponent(DeltaLocation, FQuat::Identity, true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
    if (hit->bBlockingHit) {
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("internal bounce")));
        float decimationFactor = 0.f; //FVector::DotProduct(DeltaLocation.GetSafeNormal(), hit->Normal);
        float hitMagnitude = 0.f;
        if(hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic) {
            decimationFactor = FVector::DotProduct(DeltaLocation.GetSafeNormal(), hit->Normal);
            hitMagnitude = -FVector::DotProduct(DeltaLocation, hit->Normal);
        }else if(hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldDynamic){
            AAIMachine* otherMachine = Cast<AAIMachine>(hit->GetActor());
            FVector deltaDifference = DeltaLocation - otherMachine->RealDeltaLocation;
            //otherMachine->HitByPlayer(-HitDelta, DeltaTime);
            float tempDot = FVector::DotProduct(deltaDifference, hit->Normal);
            //if(tempDot < 0){
                decimationFactor = FVector::DotProduct(deltaDifference / (MaxSpeed * DeltaTime * 10), hit->Normal);
                hitMagnitude = -tempDot;
            otherMachine->Bounce(-hit->Normal, hitMagnitude, true);
            //}
        }
        
        Bounce(hit->Normal, hitMagnitude, false);
        //HitDelta = hit->Normal * hitMagnitude * HitBounceScaler;
        //Speed += MaxSpeed * decimationFactor;
        //SpeedModifier += BoostSpeed *  decimationFactor;
        //FVector deflectedLocation = hit->Normal * 2.5f;
        //KinematicComponent->AddWorldOffset(deflectedLocation);
	}
	Raycast(DeltaTime);
	float speed = FVector::Distance(KinematicComponent->GetComponentLocation(), LastMachineLocation) / DeltaTime;//cm / seg
	speed *= 0.36f; //constant to km/h
	SpeedKH = speed;
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Speed: %f"), speed));
	CameraComponent->FieldOfView = FMath::Clamp(FMath::Lerp(CameraComponent->FieldOfView, speed/22 + 60, DeltaTime * 5), 100.f, 180.f);
	LastMachineLocation = KinematicComponent->GetComponentLocation();
    SpeedModifier -= DeltaTime * BoostDecceleration;
    SpeedModifier = FMath::Clamp(SpeedModifier, 0.f, BoostSpeed);
    if(bExternalBlock && FVector::Distance(LastHitLocation, VisibleComponent->GetComponentLocation()) > HitDistanceThereshold){
        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("exit hit")));
        bExternalBlock = false;
    }
    bBouncing = false;
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

	if (bGrounded) {
        FRotator rotationWithRoll = CameraContainerComponent->GetComponentRotation();
		if (!bDrifting) {
			rotationWithRoll.Roll += ((1 - 1 / (1 + FMath::Abs(MovementInput.X) * 4)) * FMath::Sign(MovementInput.X)
				+ (1 - 1 / (1 + FMath::Abs(RightDrift - LeftDrift) * 4)) * FMath::Sign(RightDrift - LeftDrift)) * 20;
		}
        VisibleComponent->SetWorldRotation(FMath::Lerp(VisibleComponent->GetComponentRotation(), rotationWithRoll, deltaTime * 10));
        if(bDrifting){
            DriftYaw += (MovementInput.X + (RightDrift - LeftDrift) + DriftingOffset) * deltaTime * 20;
            DriftYaw = FMath::Clamp(DriftYaw, -15.f, 15.f);
            FRotator deltaRotator = FRotator::ZeroRotator;
            deltaRotator.Yaw = DriftYaw;
            VisibleComponent->AddLocalRotation(deltaRotator);
        }
        AirYaw = rotationWithRoll.Yaw;
        AirPitch = 0;
	}
	else {
        FRotator deltaRotator = FRotator::ZeroRotator;
        deltaRotator.Yaw = (MovementInput.X + RightDrift - LeftDrift) * deltaTime * Steering;
        float smooth = 1 - FMath::Abs(AirPitch)/60;
        float delta = -MovementInput.Y * deltaTime * MaxSteering;
            AirPitch += delta;
			AirPitch = FMath::Clamp(AirPitch, -60.f, 60.f);
            deltaRotator.Pitch = delta * smooth;
        //GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Airpitch: %f"), AirPitch));
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
            
            FRotator FlatWithYaw = FRotationMatrix::MakeFromZY(ArrowComponent->GetUpVector(), VisibleComponent->GetRightVector()).Rotator();
            VisibleComponent->SetWorldRotation(FlatWithYaw);
            float deltaYaw = FlatWithYaw.Yaw - ArrowComponent->GetComponentRotation().Yaw;
            if(FMath::Abs(deltaYaw) > 10 && (Speed + SpeedModifier) > MaxSpeed * 0.9f && bCanDrift){
                bDrifting = true;
				GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
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
	if (!bDrifting && (Speed + SpeedModifier) > MaxSpeed * 0.9f && bCanDrift && MovementInput.X < 0) {
		bDrifting = true;
		bCanExitDrift = false;
		GetWorldTimerManager().SetTimer(EDTimerHandle, ExitDriftDelegate, 1.f, false, 1.f);
	}
}
void AKinematicMachine::DriftRight()
{
	if (!bDrifting && (Speed + SpeedModifier) > MaxSpeed * 0.9f && bCanDrift && MovementInput.X > 0) {
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
	if (!bBraking && bCanAccelerate) {
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
	if (Energy > BoostConsumption) {
		Energy -= BoostConsumption;
		SpeedModifier = BoostSpeed;
	}
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

void AKinematicMachine::HitByMachineSide(FVector hitNormal, FVector otherDeltaLocation, FVector lastHitLocation, float deltaTime) {
    bExternalBlock = true;
    
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("external bounce")));
    
    FVector deltaDifference = otherDeltaLocation - DeltaLocation;
    //float decimationFactor = FVector::DotProduct(deltaDifference / (MaxSpeed * deltaTime * 10), hitNormal);
    float hitMagnitude = 0.f;
    
    if(FVector::DotProduct(deltaDifference, hitNormal) > 0){
    
        hitMagnitude = FVector::DotProduct(deltaDifference, hitNormal);
        
    }
    
    //HitDelta = hitNormal * hitMagnitude * HitBounceScaler;
    //Speed += MaxSpeed * decimationFactor;
    //SpeedModifier += BoostSpeed *  decimationFactor;
    //FVector deflectedLocation = -hitNormal * 2.5f;
    //KinematicComponent->AddWorldOffset(deflectedLocation);
    
    LastHitLocation = lastHitLocation;
}
void AKinematicMachine::HitByMachineForward(float forwardDot) {
}

void AKinematicMachine::Bounce(FVector hitDirection, float hitMagnitude, bool external) {
    if(!bBouncing){
        if(external){
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Kinematic: external")));
        }else{
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Kinematic: internal")));
        }
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("K magnitude: %f"), hitMagnitude));
        bBouncing = true;
		Energy -= ShieldDamage;
		CheckDepletion();
        HitDelta = hitDirection * hitMagnitude * HitBounceScaler;
        FVector deflectedLocation = hitDirection * 2.5f;
        KinematicComponent->AddWorldOffset(deflectedLocation);
    }
}

void AKinematicMachine::StartRace() {
	bCanAccelerate = true;
	if (bPendingAcceleration) {
		bPendingAcceleration = false;
		Accelerate();
	}
}

void AKinematicMachine::CheckDepletion() {
	if (Energy <= 0) {
		bCanAccelerate = false;
		bAccelerating = false;
	}
}

