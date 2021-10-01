// Fill out your copyright notice in the Description page of Project Settings.


#include "BoostSpeedComponent.h"
#include "Components/BrushComponent.h"

UBoostSpeedComponent::UBoostSpeedComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	TriggerComponent = CreateDefaultSubobject<UBrushComponent>(TEXT("TriggerComponent"));
	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));

}


void UBoostSpeedComponent::BeginPlay()
{
	Super::BeginPlay();

	
}


void UBoostSpeedComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

