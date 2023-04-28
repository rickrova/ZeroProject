// Fill out your copyright notice in the Description page of Project Settings.


#include "GuideToSurface.h"

// Sets default values
AGuideToSurface::AGuideToSurface()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Collider = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Collider"));
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

	Arrow->SetupAttachment(Collider);

}

// Called when the game starts or when spawned
void AGuideToSurface::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGuideToSurface::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

