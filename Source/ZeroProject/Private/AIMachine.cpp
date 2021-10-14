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

	TimerDelegate.BindUFunction(this, FName("StartRace"));
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false, 3.f);

	DesiredDeltaX = DeltaX;
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
		Speed = PreSpeed + CurveFactor * curveSpeedStabilizer * DeltaTime - FMath::Clamp(VerticalSpeed, 0.f, VerticalSpeed) * DeltaTime;

		SetHeight(DeltaTime);
        FVector deltaLocation = DesiredLocation - VisibleComponent->GetComponentLocation();
		FHitResult* hit = new FHitResult();
		VisibleComponent->MoveComponent(deltaLocation, DesiredRotation, true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
		if (hit->bBlockingHit) {
			if (hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic) {
				SurfaceComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation()
                                                   - SurfaceComponent->GetRightVector() * 10 * FMath::Sign(DeltaX));
				VisibleComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation());
                DeltaX -= 10 * FMath::Sign(DeltaX);
                float newDesiredDelta = FMath::RandRange(0.f, 0.95f);
                DesiredDeltaX = DeltaX * newDesiredDelta;
				PreSpeed *= 0.9f;
			}
			else {
				AAIMachine* otherMachine = Cast<AAIMachine>(hit->GetActor());
				AKinematicMachine* playerMachine = Cast<AKinematicMachine>(hit->GetActor());
				if (otherMachine != NULL) {
                    float forwardDot = FVector::DotProduct(hit->Normal, SurfaceComponent->GetForwardVector());
                    float rightDot = FVector::DotProduct(hit->ImpactNormal, SurfaceComponent->GetRightVector());
                    if(FMath::Abs(forwardDot) > FMath::Abs(rightDot)){
						HitByMachine2(forwardDot);
						otherMachine->HitByMachine2(-forwardDot);
                    }else{
						HitByMachine(rightDot);                        
                        otherMachine->HitByMachine(-rightDot);
                    }
				}
				else if (playerMachine != NULL) {
					//playerMachine
				}
                PreSpeed *= 0.975f;
			}
		}

		if (FMath::Abs(deltaAngle) < SmartDeltaAngleTheresholdLow
            && bCanSetNewDesiredDeltaX) {
				bCanSetNewDesiredDeltaX = false;
				float rr = FMath::RandRange(-1.f, 1.f);
				DesiredDeltaX = 300 * FMath::Sign(rr);
		}
		else if(FMath::Abs(deltaAngle) > SmartDeltaAngleTheresholdHigh)
        {
            if(!bCanSetNewDesiredDeltaX) {
				bCanSetNewDesiredDeltaX = true;
			}
			if (deltaAngle * DeltaX < 0) {
				DesiredDeltaX += deltaAngle * Steering * DeltaTime;
			}
		}

        DeltaX = FMath::Lerp(DeltaX, DesiredDeltaX, DeltaTime * 0.5f);
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
}

void AAIMachine::StartRace() {
	bCanRace = true;
}

void AAIMachine::HitByMachine(float rightDot) {
	DesiredDeltaX = DeltaX + 100 * FMath::Sign(rightDot);
}
void AAIMachine::HitByMachine2(float forwardDot) {
	PreSpeed *= 1 + 0.01f * FMath::Sign(forwardDot);
}

void AAIMachine::SetHeight(float deltaTime){
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

    if (GetWorld()->LineTraceMultiByChannel(outHits, start, end, ECC_GameTraceChannel1)) {
        if (outHits.Num() > 1) {
			SurfaceComponent->SetWorldLocation(outHits[1].ImpactPoint);
		}
		else {
			SurfaceComponent->SetWorldLocation(outHits[0].ImpactPoint);
		}

        if(outHits[0].Distance < TraceOffset + DistanceToFloor){
			if (!bGrounded) {
				bGrounded = true;
				if(bDebug){
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Landing")));
				}
			}
            DynamicComponent->SetWorldLocation(outHits[0].ImpactPoint);
			DesiredLocation = outHits[0].ImpactPoint + outHits[0].Normal * DistanceToFloor;
            DesiredRotation = FRotationMatrix::MakeFromZX(outHits[0].Normal, SurfaceComponent->GetForwardVector()).Rotator();

			float currentHeight = FVector::Distance(outHits[0].ImpactPoint, SurfaceComponent->GetComponentLocation());
			LastVerticalDelta = currentHeight - LastHeight;
			LastHeight = currentHeight;
			VerticalSpeed = LastVerticalDelta * 1.f;
        }else{
			if (bGrounded) {
				bGrounded = false;
				if (bDebug) {
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("On air")));
				}
			}
            VerticalSpeed -= Gravity * deltaTime * deltaTime;
            LastHeight += VerticalSpeed;
            DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * LastHeight);
			DesiredLocation = DynamicComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor;
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
		DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * (LastHeight ));
		DesiredLocation = DynamicComponent->GetComponentLocation() -gravityDirection * DistanceToFloor;
	}
	LastSurfaceLocation = Guide->GetSocketLocation("BoneSocket");
}

