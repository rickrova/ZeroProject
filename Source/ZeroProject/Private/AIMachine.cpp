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

	VisibleComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AAIMachine::BeginPlay()
{
	Super::BeginPlay();

	TargetComponent = Target->FindComponentByClass<USkeletalMeshComponent>(); 
}

// Called every frame
void AAIMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Speed += AccelerationRate * DeltaTime * DeltaTime;
	Speed = FMath::Clamp(Speed, 0.f, MaxSpeed);

	RootComponent->SetWorldLocation(TargetComponent->GetSocketLocation("BoneSocket"));
	RootComponent->SetWorldRotation(TargetComponent->GetSocketRotation("BoneSocket"));
}

