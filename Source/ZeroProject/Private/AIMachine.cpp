// Fill out your copyright notice in the Description page of Project Settings.


#include "AIMachine.h"
#include "../KinematicMachine.h" // /Rick/Projects/ZeroProject/Unreal/Source/ZeroProject/KinematicMachine.h"
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

		//FVector rightDirection = SurfaceComponent->GetRightVector();
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
		Speed = CurveFactor * curveSpeedStabilizer * DeltaTime + PreSpeed;

		//FVector desiredLocation = Guide->GetSocketLocation("BoneSocket") + SurfaceComponent->GetRightVector() * DeltaX;
		//FVector flatDeltaLocation = FVector::VectorPlaneProject(deltaLocation, SurfaceComponent->GetUpVector()) + DesiredVerticalMovement;

		SetHeight(DeltaTime);

		//FVector deltaLocation = SurfaceComponent->GetComponentLocation() - VisibleComponent->GetComponentLocation();
        //FVector deltaLocation = DesiredLocation - VisibleComponent->GetComponentLocation();
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
				//DeltaX *= 0.95f;
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
				//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("delta change: %f"), rr));
				//DeltaX = DesiredDeltaX;
			//DeltaX = FMath::Lerp(DeltaX, DesiredDeltaX, DeltaTime * 1.025f);
		}
		else if(FMath::Abs(deltaAngle) > SmartDeltaAngleTheresholdHigh)
        {
            if(!bCanSetNewDesiredDeltaX) {
				bCanSetNewDesiredDeltaX = true;
			}
            DesiredDeltaX += deltaAngle * Steering * DeltaTime;
		}
        
        /*float horSpeed = DeltaTime * 200.f;

        if (DeltaX < DesiredDeltaX) {
            DeltaX += horSpeed;
        }
        else if (DeltaX > DesiredDeltaX) {
            DeltaX -= horSpeed;
        }
        if (FMath::Abs(DeltaX - DesiredDeltaX) < horSpeed) {
            DeltaX = DesiredDeltaX;
        }*/
        DeltaX = FMath::Lerp(DeltaX, DesiredDeltaX, DeltaTime * 0.5f);
		LastDirection = SurfaceComponent->GetForwardVector();
	}
	else {
		SurfaceComponent->SetWorldLocation(Guide->GetSocketLocation("BoneSocket") + SurfaceComponent->GetRightVector() * DeltaX);
		SurfaceComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
        DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation());
        DynamicComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
		SetHeight(DeltaTime);
		VisibleComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * DistanceToFloor);
		VisibleComponent->SetWorldRotation(SurfaceComponent->GetComponentRotation());
	}
}

void AAIMachine::StartRace() {
	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Start race")));
	bCanRace = true;
}

void AAIMachine::HitByMachine(float rightDot) {
	//SurfaceComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetRightVector() * 10 * FMath::Sign(rightDot));
	//DeltaX += 10 * FMath::Sign(rightDot);
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
	//DynamicComponent->SetWorldLocation(SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * LastHeight);

	TArray<FHitResult> outHits;
    FVector gravityDirection = -SurfaceComponent->GetUpVector();
    FVector start = SurfaceComponent->GetComponentLocation() + SurfaceComponent->GetUpVector() * (LastHeight + 200);
    FVector end = start + gravityDirection * 400;
    //DrawDebugLine(GetWorld(), start, end, FColor::Orange, false, 0.1f);

    if (GetWorld()->LineTraceMultiByChannel(outHits, start, end, ECC_GameTraceChannel1)) {
        //gravityDirection = - outHits[0].Normal;
        /*if(bDebug){
            
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Distance: %f"), outHits[0].Distance));
        }*/

		if (outHits.Num() > 1) {
			SurfaceComponent->SetWorldLocation(outHits[1].ImpactPoint);
		}
		else {
			SurfaceComponent->SetWorldLocation(outHits[0].ImpactPoint);
		}

        if(outHits[0].Distance < 220){
			if (!bGrounded) {
				bGrounded = true;
				if(bDebug){
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Landing")));
				}
			}
            //DesiredVerticalMovement = FVector::ZeroVector;
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
        
        
    /*
        
        if (hit->Distance > 200) {            
            VerticalSpeed += Gravity * deltaTime * deltaTime;
			if (VerticalSpeed + 200 < hit->Distance) {
				DesiredVerticalMovement = gravityDirection * VerticalSpeed;
                VerticalOffset = FVector::Distance(VisibleComponent->GetComponentLocation(),
                                                   SurfaceComponent->GetComponentLocation())
                - VerticalSpeed;
			}
			else {
				VerticalSpeed = 0;
				DesiredVerticalMovement = FVector::ZeroVector; //-gravityDirection * (200 - hit->Distance);
				DesiredRotation = FRotationMatrix::MakeFromZX(hit->Normal, SurfaceComponent->GetForwardVector()).Rotator();
                VerticalOffset = MinVerticalOffset;
			}
        }else{
            VerticalSpeed = 0;
			DesiredVerticalMovement = FVector::ZeroVector; // -gravityDirection * (200 - hit->Distance);
			DesiredRotation = FRotationMatrix::MakeFromZX(hit->Normal, SurfaceComponent->GetForwardVector()).Rotator();
            VerticalOffset = MinVerticalOffset;
        }
        DesiredLocation = hit->ImpactPoint + hit->Normal * VerticalOffset;
        FVector deltaLocation = DesiredLocation - VisibleComponent->GetComponentLocation();
        FlatDelta = deltaLocation; //FVector::VectorPlaneProject(deltaLocation, SurfaceComponent->GetUpVector());
        
        */

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
        //DesiredVerticalMovement = gravityDirection * VerticalSpeed;
        
		//VerticalSpeed += Gravity * deltaTime * deltaTime;
		//DesiredVerticalMovement = gravityDirection * VerticalSpeed;
	}
	LastSurfaceLocation = Guide->GetSocketLocation("BoneSocket");
}

