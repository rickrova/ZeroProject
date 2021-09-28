// Fill out your copyright notice in the Description page of Project Settings.


#include "AIMachine.h"
#include "Engine/SkeletalMeshSocket.h"

// Sets default values
AAIMachine::AAIMachine()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));
	Guide = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Guide"));

	VisibleComponent->SetupAttachment(RootComponent);
	Guide->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AAIMachine::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AAIMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    VisibleComponent->SetWorldRotation(Guide->GetSocketRotation("BoneSocket"));
    CurrentDirection = VisibleComponent->GetForwardVector();
    FVector rightDirection = VisibleComponent->GetRightVector();
    AngleAlpha = FMath::RadiansToDegrees(acosf(FVector::DotProduct(rightDirection, LastDirection)));
    AngleBeta = 180 - AngleAlpha;

	PreSpeed += AccelerationRate * DeltaTime * DeltaTime;
	PreSpeed = FMath::Clamp(PreSpeed, 0.f, MaxSpeed);
    float aditionalSpeed = (FMath::Atan(DeltaX)/2 + 1)*(AngleAlpha - AngleBeta);
    Speed = aditionalSpeed + PreSpeed;

	FVector desiredPosition = Guide->GetSocketLocation("BoneSocket")
    + VisibleComponent->GetRightVector() * DeltaX;

	VisibleComponent->SetWorldLocation(desiredPosition);
    LastDirection = CurrentDirection;
}

