// Fill out your copyright notice in the Description page of Project Settings.

#include "AIMachine.h"
#include "../KinematicMachine.h" 
#include "Engine/SkeletalMeshSocket.h"
#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h"

AAIMachine::AAIMachine()
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

void AAIMachine::BeginPlay()
{
    Super::BeginPlay();

    //TimerDelegate.BindUFunction(this, FName("StartRace"));
    //GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false, 3.f);

    DesiredDeltaX = DeltaX;
    CoveredDistance = StartPosition * CurveLength / AnimationTime;
}

void AAIMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AAIMachine::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bCanRace) {
        RealDeltaLocation = Guide->GetSocketLocation("BoneSocket") - LastSurfaceLocation;
        SurfaceComponent->SetWorldLocation(Guide->GetSocketLocation("BoneSocket") + SurfaceComponent->GetRightVector() * DeltaX);
        SurfaceComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));

        FVector flatLastDirection = FVector::VectorPlaneProject(LastDirection, SurfaceComponent->GetUpVector());
        flatLastDirection.Normalize();
        float epsilonX = FVector::DotProduct(SurfaceComponent->GetForwardVector(), flatLastDirection);
        float sign = -FMath::Sign(FVector::DotProduct(SurfaceComponent->GetRightVector(), flatLastDirection));
        float deltaAngle = FMath::RadiansToDegrees(FMath::Acos(epsilonX)) * sign;

        float curveSpeedStabilizer = FMath::Sin(deltaAngle) * DeltaX;
        float deltaSpeed = AccelerationRate * DeltaTime * DeltaTime;

        if (PreSpeed < MaxSpeed) {
            PreSpeed += deltaSpeed;
        }
        else if (PreSpeed > MaxSpeed) {
            PreSpeed -= deltaSpeed;
        }
        if (FMath::Abs(PreSpeed - MaxSpeed) < deltaSpeed) {
            PreSpeed = MaxSpeed;
        }
        //PreSpeed = MaxSpeed;
        CoveredDistance += Speed; // / normalizer * 60.f;

        Speed = (PreSpeed + CurveFactor * curveSpeedStabilizer - FMath::Clamp(VerticalSpeed, 0.f, VerticalSpeed)
            + FVector::DotProduct(SurfaceComponent->GetForwardVector(), HitDelta) * 0.1f + SpeedModifier) * DeltaTime;

        //Speed = MaxSpeed * DeltaTime;

        SetHeight(DeltaTime);
        HitDelta = FMath::Lerp(HitDelta, FVector::ZeroVector, DeltaTime * HitDecceleration);
        //DesiredLocation += HitDelta;
        FVector desiredDeltaLocation = DesiredLocation - VisibleComponent->GetComponentLocation();
        float normalizer = CurveLength / AnimationTime;

        FHitResult* hit = new FHitResult();
        VisibleComponent->MoveComponent(desiredDeltaLocation, DesiredRotation, true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
        if (hit->bBlockingHit) {

            if (hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic) {

                //HitDelta += hit->Normal * HitBounceScaler;
                float hitOffset = FVector::DotProduct(hit->Normal, SurfaceComponent->GetForwardVector());
                SurfaceComponent->AddWorldOffset(SurfaceComponent->GetRightVector() * hitOffset * 100.f * FMath::Sign(DeltaX));
                FVector offset = DesiredLocation + SurfaceComponent->GetRightVector() * hitOffset * 100.f * FMath::Sign(DeltaX) - VisibleComponent->GetComponentLocation();
                VisibleComponent->AddWorldOffset(offset);
                DeltaX += hitOffset * 100.f * FMath::Sign(DeltaX);

                float newDesiredDelta = FMath::RandRange(0.f, 0.95f);
                DesiredDeltaX = DeltaX * newDesiredDelta;
                //PreSpeed *= 0.9f;
                //PreSpeed += FVector::DotProduct(SurfaceComponent->GetForwardVector(), HitDelta) * DeltaTime;

                Bounce(hit->Normal, 6.f, false);
            }
            else if (hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldDynamic) {

                /*float decimationFactor = 0.f;
                float hitMagnitude = 0.f;

                AAIMachine* otherMachine = Cast<AAIMachine>(hit->GetActor());
                AKinematicMachine* playerMachine = Cast<AKinematicMachine>(hit->GetActor());

                if (otherMachine != NULL) {
                    FVector deltaDifference = RealDeltaLocation - otherMachine->RealDeltaLocation;
                    decimationFactor = FVector::DotProduct(deltaDifference / RealDeltaLocation.Size(), hit->Normal);
                    hitMagnitude = -FVector::DotProduct(deltaDifference, hit->Normal);

                    //HitDelta = hit->Normal * hitMagnitude * HitBounceScaler;
                    //PreSpeed += MaxSpeed * decimationFactor;
                }else if(playerMachine != NULL) {
                    FVector deltaDifference = RealDeltaLocation - playerMachine->DeltaLocation;
                    decimationFactor = FVector::DotProduct(deltaDifference / RealDeltaLocation.Size(), hit->Normal);
                    hitMagnitude = -FVector::DotProduct(deltaDifference, hit->Normal);

                    //HitDelta = hit->Normal * hitMagnitude * HitBounceScaler;
                    //PreSpeed += MaxSpeed * decimationFactor;
                }*/

                AAIMachine* otherMachine = Cast<AAIMachine>(hit->GetActor());
                AKinematicMachine* playerMachine = Cast<AKinematicMachine>(hit->GetActor());
                if (otherMachine != NULL) {

                    SurfaceComponent->AddWorldOffset(-SurfaceComponent->GetRightVector() * 2.5f * FMath::Sign(DeltaX));
                    FVector offset = DesiredLocation - SurfaceComponent->GetRightVector() * 2.5f * FMath::Sign(DeltaX) - VisibleComponent->GetComponentLocation();
                    VisibleComponent->AddWorldOffset(offset);
                    DeltaX -= 2.5f * FMath::Sign(DeltaX);

                    float newDesiredDelta = FMath::RandRange(0.f, 0.95f);
                    DesiredDeltaX = DeltaX * newDesiredDelta;

                    float forwardDot = FVector::DotProduct(hit->Normal, SurfaceComponent->GetForwardVector());
                    float rightDot = FVector::DotProduct(hit->ImpactNormal, SurfaceComponent->GetRightVector());
                    HitDelta += hit->Normal * HitBounceScaler;
                    HitByMachine2(forwardDot);
                    otherMachine->HitByMachine2(-forwardDot);
                    HitByMachine(rightDot);
                    otherMachine->HitByMachine(-rightDot);
                    /*if(FMath::Abs(forwardDot) > FMath::Abs(rightDot)){
                        HitByMachine2(forwardDot);
                        otherMachine->HitByMachine2(-forwardDot);
                    }else{
                        HitByMachine(rightDot);
                        otherMachine->HitByMachine(-rightDot);
                    }*/
                    /*FVector deltaDifference = RealDeltaLocation - otherMachine->RealDeltaLocation;
                    float hitMagnitude = FVector::DotProduct(deltaDifference, hit->Normal);
                    Bounce(hit->Normal, hitMagnitude, false);
                    otherMachine->Bounce(-hit->Normal, hitMagnitude, true);*/
                }
                else if (playerMachine != NULL) {
                    /*float forwardDot = FVector::DotProduct(hit->Normal, SurfaceComponent->GetForwardVector());
                    float rightDot = FVector::DotProduct(hit->ImpactNormal, SurfaceComponent->GetRightVector());
                    HitDelta += hit->Normal * HitBounceScaler;
                    if(FMath::Abs(forwardDot) > FMath::Abs(rightDot)){
                        HitByMachine2(forwardDot);
                        //playerMachine->HitByMachineSide(-hit->Normal, RealDeltaLocation, hit->ImpactPoint, DeltaTime);
                    }else{
                        HitByMachine(rightDot);
                        //playerMachine->HitByMachineSide(-hit->Normal, RealDeltaLocation, hit->ImpactPoint, DeltaTime);
                    }*/

                    SurfaceComponent->AddWorldOffset(-SurfaceComponent->GetRightVector() * 2.5f * FMath::Sign(DeltaX));
                    FVector offset = DesiredLocation - SurfaceComponent->GetRightVector() * 2.5f * FMath::Sign(DeltaX) - VisibleComponent->GetComponentLocation();
                    VisibleComponent->AddWorldOffset(offset);
                    DeltaX -= 2.5f * FMath::Sign(DeltaX);

                    FVector deltaDifference = playerMachine->DeltaLocation - RealDeltaLocation;
                    float hitMagnitude = FVector::DotProduct(deltaDifference, hit->Normal);
                    Bounce(hit->Normal, hitMagnitude, false);
                    playerMachine->Bounce(-hit->Normal, hitMagnitude, true);
                }
                //PreSpeed *= 0.975f;
                //PreSpeed += FVector::DotProduct(SurfaceComponent->GetForwardVector(), HitDelta) * DeltaTime;
            }
        }

        if (FMath::Abs(deltaAngle) < SmartDeltaAngleTheresholdLow
            && bCanSetNewDesiredDeltaX) {
            bCanSetNewDesiredDeltaX = false;
            float rr = FMath::RandRange(-1.f, 1.f);
            DesiredDeltaX = 300 * FMath::Sign(rr);
            float randomNormalized = FMath::RandRange(0.f, 1.f);
            if (randomNormalized < BoostChance * Energy && Energy > BoostConsumption) {
                Energy -= BoostConsumption;
                SpeedModifier = BoostSpeed;
            }
        }
        else if (FMath::Abs(deltaAngle) > SmartDeltaAngleTheresholdHigh &&
            !bCanSetNewDesiredDeltaX) {
            bCanSetNewDesiredDeltaX = true;
            if (deltaAngle * DeltaX < 0) {
                DesiredDeltaX = DeltaX + 300 / 2 * FMath::Sign(deltaAngle);
            }
        }

        DeltaX = FMath::Lerp(DeltaX, DesiredDeltaX, DeltaTime * Steering) + FVector::DotProduct(SurfaceComponent->GetRightVector(), HitDelta) * DeltaTime;
        //+ HitDelta.ProjectOnToNormal(SurfaceComponent->GetRightVector()).Size() * DeltaTime;
        LastDirection = SurfaceComponent->GetForwardVector();
    }
    else {
        SurfaceComponent->SetWorldLocation(Guide->GetSocketLocation("BoneSocket") + SurfaceComponent->GetRightVector() * DeltaX);
        SurfaceComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
        DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation());
        DynamicComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
        VisibleComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor);
        VisibleComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
        SetHeight(DeltaTime);
    }
    SpeedModifier -= DeltaTime * BoostDecceleration;
    SpeedModifier = FMath::Clamp(SpeedModifier, 0.f, BoostSpeed);
    LastSurfaceLocation = Guide->GetSocketLocation("BoneSocket");
    bBouncing = false;
}

void AAIMachine::StartRace() {
    bCanRace = true;
}

void AAIMachine::SetHeight(float deltaTime) {
    FVector locationDelta = LastSurfaceLocation - Guide->GetSocketLocation("BoneSocket");
    FVector verticalProjection = locationDelta.ProjectOnToNormal(SurfaceComponent->GetUpVector());
    if (verticalProjection.Size() > ContinuityThereshold) {
        if (bDebug) {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Discontinuity")));
        }
        LastHeight = verticalProjection.Size();
    }

    DynamicComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());

    TArray<FHitResult> outHits;
    FVector gravityDirection = -SurfaceComponent->GetUpVector();
    FVector start = SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * (LastHeight + TraceOffset);
    FVector end = SurfaceComponent->GetComponentLocation() + gravityDirection * TraceOffset;
    //DrawDebugLine(GetWorld(), start, end, FColor::Orange, false, 0.f);

    if (GetWorld()->LineTraceMultiByChannel(outHits, start, end, ECC_GameTraceChannel2)) {
        if (outHits.Num() > 1) {
            SurfaceComponent->SetWorldLocation(outHits[1].ImpactPoint);
        }
        else {
            SurfaceComponent->SetWorldLocation(outHits[0].ImpactPoint);
        }

        if (outHits[0].Distance < TraceOffset + DistanceToFloor) {
            if (!bGrounded) {
                bGrounded = true;
                if (bDebug) {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Landing")));
                }
            }
            DynamicComponent->SetWorldLocation(outHits[0].ImpactPoint);
            DesiredLocation = outHits[0].ImpactPoint + outHits[0].Normal * DistanceToFloor; // + HitDelta * deltaTime;
            DesiredRotation = FRotationMatrix::MakeFromZX(outHits[0].Normal, SurfaceComponent->GetForwardVector()).Rotator();

            float currentHeight = FVector::Distance(outHits[0].ImpactPoint, SurfaceComponent->GetComponentLocation());
            LastVerticalDelta = currentHeight - LastHeight;
            LastHeight = currentHeight;
            VerticalSpeed = LastVerticalDelta * 1.f;
        }
        else {
            if (bGrounded) {
                bGrounded = false;
                if (bDebug) {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("On air")));
                }
            }
            VerticalSpeed -= Gravity * deltaTime * deltaTime;
            LastHeight += VerticalSpeed;
            DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * LastHeight);
            DesiredLocation = DynamicComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor; // + HitDelta * deltaTime;
        }
    }
    else {
        if (bGrounded) {
            bGrounded = false;
            if (bDebug) {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("On air")));
            }
        }
        VerticalSpeed -= Gravity * deltaTime * deltaTime;
        LastHeight += VerticalSpeed;
        DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * (LastHeight));
        DesiredLocation = DynamicComponent->GetComponentLocation() - gravityDirection * DistanceToFloor; // + HitDelta * deltaTime;
    }
}

void AAIMachine::HitByMachine(float rightDot) {
    DesiredDeltaX = DeltaX + 100 * FMath::Sign(rightDot);
    Energy -= ShieldDamage;
    CheckDepletion();
}
void AAIMachine::HitByMachine2(float forwardDot) {
    PreSpeed *= 1 + 0.01f * FMath::Sign(forwardDot);
    Energy -= ShieldDamage;
    CheckDepletion();
}
void AAIMachine::HitByPlayer(FVector hitDelta, float deltaTime) {
    //HitDelta += hitDelta;
    //PreSpeed += FVector::DotProduct(SurfaceComponent->GetForwardVector(), HitDelta) * deltaTime;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("hit by player")));
    Energy -= ShieldDamage;
    CheckDepletion();
}

void AAIMachine::Bounce(FVector hitDirection, float hitMagnitude, bool external) {
    if (!bBouncing) {
        if (bDebug) {
            if (external) {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("AI: external")));
            }
            else {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("AI: internal")));
            }
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("AI magnitude: %f"), hitMagnitude));
        }
        Energy -= ShieldDamage;
        CheckDepletion();
        bBouncing = true;
        HitDelta = hitDirection * hitMagnitude * HitBounceScaler;

        //int sign = FMath::Sign(FVector::DotProduct(hitDirection, SurfaceComponent->GetRightVector()));

        //SurfaceComponent->AddWorldOffset(SurfaceComponent->GetRightVector() * 20.5f * sign);
        //VisibleComponent->AddWorldOffset(SurfaceComponent->GetRightVector() * 20.5f * sign);
        //DeltaX += 20.5f * FMath::Sign(DeltaX);
        //DesiredDeltaX = DeltaX;
    }
}

void AAIMachine::CheckDepletion() {
    if (Energy <= 0) {
        //Destroy();
        MaxSpeed = 0.f;
    }
}

