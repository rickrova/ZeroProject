// Fill out your copyright notice in the Description page of Project Settings.


#include "AIMachine.h"
#include "../KinematicMachine.h" // /Rick/Projects/ZeroProject/Unreal/Source/ZeroProject/KinematicMachine.h"
#include "Engine/SkeletalMeshSocket.h"

AAIMachine::AAIMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));
	Guide = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Guide"));

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
		VisibleComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
		FVector rightDirection = VisibleComponent->GetRightVector();

		FVector flatLastDirection = FVector::VectorPlaneProject(LastDirection, VisibleComponent->GetUpVector());
		flatLastDirection.Normalize();
		float epsilonX = FVector::DotProduct(VisibleComponent->GetForwardVector(), flatLastDirection);
		float sign = -FMath::Sign(FVector::DotProduct(VisibleComponent->GetRightVector(), flatLastDirection));
		float deltaAngle = FMath::RadiansToDegrees(FMath::Acos(epsilonX)) * sign;
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("angle: %f"), deltaAngle));
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

		FVector desiredPosition = Guide->GetSocketLocation("BoneSocket")
			+ VisibleComponent->GetRightVector() * DeltaX;
		FVector deltaLocation = desiredPosition - VisibleComponent->GetComponentLocation();

		FHitResult* hit = new FHitResult();
		VisibleComponent->MoveComponent(deltaLocation, Guide->GetSocketRotation("BoneSocket"), true, hit, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::None);
		if (hit->bBlockingHit) {
			if (hit->GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic) {
                VisibleComponent->SetWorldLocation(VisibleComponent->GetComponentLocation()
                                                   - VisibleComponent->GetRightVector() * 10 * FMath::Sign(DeltaX));
                DeltaX -= 10 * FMath::Sign(DeltaX);
                DesiredDeltaX = DeltaX * 0.95f;
				PreSpeed *= 0.95f;
			}
			else {
				//DeltaX *= 0.95f;
				AAIMachine* otherMachine = Cast<AAIMachine>(hit->GetActor());
				AKinematicMachine* playerMachine = Cast<AKinematicMachine>(hit->GetActor());
				if (otherMachine != NULL) {
                    float forwardDot = FVector::DotProduct(hit->Normal, -VisibleComponent->GetForwardVector());
                    float rightDot = FVector::DotProduct(hit->Normal, -VisibleComponent->GetRightVector());
                    if(FMath::Abs(forwardDot) > FMath::Abs(rightDot)){
                        otherMachine->PreSpeed = PreSpeed * 1.025;
                    }else{
                        //float tempDesiredDeltaX = DesiredDeltaX;
                        //VisibleComponent->SetWorldLocation(VisibleComponent->GetComponentLocation()
                                                           //- VisibleComponent->GetRightVector() * 10 * FMath::Sign(rightDot));
                        //DeltaX = 10 * FMath::Sign(rightDot);
                        DesiredDeltaX = DeltaX;
                        
                        otherMachine->HitByMachine();
                    }
				}
				else if (playerMachine != NULL) {
					//playerMachine
				}
                PreSpeed *= 0.975f;
			}
		}

		if (FMath::Abs(deltaAngle) < SmartDeltaAngleThereshold
            && bCanSetNewDesiredDeltaX) {
				bCanSetNewDesiredDeltaX = false;
				float rr = FMath::RandRange(-1.f, 1.f);
				DesiredDeltaX = 300 * FMath::Sign(rr);
				//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("delta change: %f"), rr));
				//DeltaX = DesiredDeltaX;
			//DeltaX = FMath::Lerp(DeltaX, DesiredDeltaX, DeltaTime * 1.025f);
		}
		else if(FMath::Abs(deltaAngle) > SmartDeltaAngleThereshold)
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
		LastDirection = VisibleComponent->GetForwardVector();
	}
	else {
		VisibleComponent->SetWorldLocation(Guide->GetSocketLocation("BoneSocket") + VisibleComponent->GetRightVector() * DeltaX);
		VisibleComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
	}
}

void AAIMachine::StartRace() {
	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Start race")));
	bCanRace = true;
}

void AAIMachine::HitByMachine() {
    DesiredDeltaX = DeltaX;
}

