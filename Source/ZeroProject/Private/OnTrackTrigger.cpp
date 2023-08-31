// Fill out your copyright notice in the Description page of Project Settings.


#include "OnTrackTrigger.h"
#include "AdaptativeMachine.h"
#include "PlayerMachine.h"

// Sets default values
AOnTrackTrigger::AOnTrackTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	VisibleComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleComponent"));

	SetRootComponent(Trigger);
	VisibleComponent->SetupAttachment(Trigger);
}

void AOnTrackTrigger::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AOnTrackTrigger::OnTriggerBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AOnTrackTrigger::OnTriggerEndOverlap);
}

void AOnTrackTrigger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AOnTrackTrigger::BoostBehavior(AActor * OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->ExternalBoost(BoostRatio);
	}
	else {
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->ExternalBoost(BoostRatio);
	}
}

void AOnTrackTrigger::JumpBehavior(AActor* OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->Jump(JumpRatio);
	}
	else {
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->Jump(JumpRatio);
	}
}

void AOnTrackTrigger::SlowBehavior(AActor* OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->Slow(SlowRatio);
	}
	else {
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->Slow(SlowRatio);
	}
}

void AOnTrackTrigger::ResetSlowBehavior(AActor* OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->ResetSlow(SlowRatio);
	}
	else {
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->ResetSlow(SlowRatio);
	}
}

void AOnTrackTrigger::StartEnergyTransmission(AActor* OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->StartEnergyTransmission(EnergyRatio);
	}
	else {
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->StartEnergyTransmission(EnergyRatio);
	}
}

void AOnTrackTrigger::EndEnergyTransmission(AActor* OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->EndEnergyTransmission();
	}
	else {
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->EndEnergyTransmission();
	}
}


void AOnTrackTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (bBoost) {
		BoostBehavior(OtherActor);
	}
	else if (bSlow) {
		SlowBehavior(OtherActor);
	}
	if (bJump) {
		JumpBehavior(OtherActor);
	}
	if (bEnergyModifier) {
		StartEnergyTransmission(OtherActor);
	}
}

void AOnTrackTrigger::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {
	if (bSlow) {
		ResetSlowBehavior(OtherActor);
	}
	if (bEnergyModifier) {
		EndEnergyTransmission(OtherActor);
	}
}

