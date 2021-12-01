// Fill out your copyright notice in the Description page of Project Settings.

#include "AIMachine_v2.h"
#include "../KinematicMachine.h" 
#include "Engine/SkeletalMeshSocket.h"
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
        Move();
    }
}

void AAIMachine_v2::Init() {
    SurfaceComponent->SetWorldLocation(Guide->GetSocketLocation("BoneSocket") + DeltaXY);
    SurfaceComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
    DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation());
    DynamicComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
    VisibleComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor);
    VisibleComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
}
void AAIMachine_v2::Move() {
    FHitResult* hit = new FHitResult();
    FVector traceStart = Guide->GetSocketLocation("BoneSocket") + DeltaXY + SurfaceComponent->GetUpVector() * TraceOffset * 0.5f;
    FVector traceEnd = traceStart - SurfaceComponent->GetUpVector() * TraceOffset;

    if (GetWorld()->LineTraceSingleByChannel(*hit, traceStart, traceEnd, ECC_GameTraceChannel1)) {
        SurfaceComponent->SetWorldLocation(hit->ImpactPoint);
        SurfaceComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
        FVector forwardAxis = FRotationMatrix(Guide->GetSocketRotation("BoneSocket")).GetScaledAxis(EAxis::X);
        SurfaceComponent->SetWorldRotation(FRotationMatrix::MakeFromZX(hit->Normal, forwardAxis).Rotator());
        DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation());
        DynamicComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
        VisibleComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor);
        VisibleComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
    }
}

void AAIMachine_v2::SetOrientation() {
}

void AAIMachine_v2::StartRace() {
    bCanRace = true;
}

void AAIMachine_v2::CheckDepletion() {
    if (Energy <= 0) {
        MaxSpeed = 0.f;
    }
}

