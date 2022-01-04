// Fill out your copyright notice in the Description page of Project Settings.

#include "AIMachine_v2.h"
#include "../KinematicMachine.h" 
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h"

AAIMachine_v2::AAIMachine_v2()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));
    SurfaceComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("SurfaceComponent"));
    DynamicComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("DynamicComponent"));
    Guide = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Guide"));

    SurfaceComponent->SetupAttachment(RootComponent);
    DynamicComponent->SetupAttachment(RootComponent);
    VisibleComponent->SetupAttachment(RootComponent);
    Guide->SetupAttachment(RootComponent);
}

void AAIMachine_v2::BeginPlay()
{
    Super::BeginPlay();
}

void AAIMachine_v2::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AAIMachine_v2::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bCanRace) {
        Move(DeltaTime);
        CalculateOnRailSpeed(DeltaTime);
    }
}

void AAIMachine_v2::Init() {
    SurfaceComponent->SetWorldLocation(Guide->GetSocketLocation("BoneSocket") + DeltaLocationX + DeltaLocationY);
    SurfaceComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
    DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation());
    DynamicComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
    VisibleComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor);
    VisibleComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
}

void AAIMachine_v2::Drive(float deltaTime){
    float epsilonX = FVector::DotProduct(SurfaceComponent->GetForwardVector(), LastDirection);
    //float sign = -FMath::Sign(FVector::DotProduct(SurfaceComponent->GetRightVector(), flatLastDirection));
    float deltaAngle = FMath::RadiansToDegrees(FMath::Acos(epsilonX)); // * sign;

    if (FMath::Abs(deltaAngle) < SmartDeltaAngleTheresholdLow
        && bCanSetNewDesiredDeltaX) {
        bCanSetNewDesiredDeltaX = false;
        float rr = FMath::RandRange(-1.f, 1.f);
        //TempDeltaX = 0;
        DesiredDeltaX = 100 * rr;
//        float randomNormalized = FMath::RandRange(0.f, 1.f);
//        if (randomNormalized < BoostChance * Energy && Energy > BoostConsumption) {
//            Energy -= BoostConsumption;
//            SpeedModifier = BoostSpeed;
//        }
    }
    else if (FMath::Abs(deltaAngle) > SmartDeltaAngleTheresholdHigh &&
        !bCanSetNewDesiredDeltaX) {
        bCanSetNewDesiredDeltaX = true;
//        if (deltaAngle * DeltaX < 0) {
//            DesiredDeltaX = DeltaX + 300 / 2 * FMath::Sign(deltaAngle);
//        }
    }
    
    TempDeltaX = FMath::Lerp(TempDeltaX, DesiredDeltaX, deltaTime * Steering);
    DeltaDeltaX = TempDeltaX - LastDeltaX;
    LastDeltaX = TempDeltaX;
    
    DeltaLocationX = SurfaceComponent->GetRightVector() * DeltaDeltaX;
    DeltaLocationY = FVector::ZeroVector;
    
    /*
    TempDeltaX = FMath::Lerp(TempDeltaX, DesiredDeltaX, deltaTime * Steering);
    DeltaDeltaX = TempDeltaX - DeltaX;
    DeltaX = FMath::Lerp(DeltaX, DesiredDeltaX, deltaTime * Steering); //+ FVector::DotProduct(SurfaceComponent->GetRightVector(), HitDelta) * deltaTime;
    //DeltaXY += SurfaceComponent->GetRightVector() * deltaDeltaX; //DeltaX;
    
    */
    LastDirection = SurfaceComponent->GetForwardVector();
}

void AAIMachine_v2::Move(float deltaTime) {
    FHitResult* hit = new FHitResult();
    FVector traceStart = Guide->GetSocketLocation("BoneSocket")
    + DeltaLocationX
    + DeltaLocationY + SurfaceComponent->GetUpVector() * TraceOffset * 0.5f;
    FVector traceEnd = traceStart - SurfaceComponent->GetUpVector() * TraceOffset;
    FVector forwardAxis = FRotationMatrix(Guide->GetSocketRotation("BoneSocket")).GetScaledAxis(EAxis::X);

    DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Green, false);
    if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECC_GameTraceChannel1)) {
        //DeltaXY = hit->ImpactPoint - Guide->GetSocketLocation("BoneSocket");
    }

    FRotator surfaceOrientation = FRotationMatrix::MakeFromZX(hit->Normal, forwardAxis).Rotator();
    SurfaceComponent->SetWorldRotation(surfaceOrientation);
    Drive(deltaTime);
    SurfaceComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + DeltaLocationX + DeltaLocationY);
    DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation());
    DynamicComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());

    FVector desiredDeltaLocation = SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor - VisibleComponent->GetComponentLocation();
    VisibleComponent->MoveComponent(desiredDeltaLocation, SurfaceComponent->GetComponentRotation(), true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
    if (hit->bBlockingHit) {
    }
    LocalDeltaXY = SurfaceComponent->GetComponentLocation();
}

void AAIMachine_v2::SetOrientation() {
}

void AAIMachine_v2::CalculateOnRailSpeed(float deltaTime) {
    float deltaSpeed = AccelerationRate * deltaTime * deltaTime;
    if (PreSpeed < MaxSpeed) {
        PreSpeed += deltaSpeed;
    }
    else if (PreSpeed > MaxSpeed) {
        PreSpeed -= deltaSpeed;
    }
    if (FMath::Abs(PreSpeed - MaxSpeed) < deltaSpeed) {
        PreSpeed = MaxSpeed;
    }
    CoveredDistance += Speed;

    Speed = PreSpeed;
}

void AAIMachine_v2::StartRace() {
    bCanRace = true;
}

void AAIMachine_v2::CheckDepletion() {
    if (Energy <= 0) {
        MaxSpeed = 0.f;
    }
}

