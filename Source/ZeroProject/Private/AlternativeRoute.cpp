// Fill out your copyright notice in the Description page of Project Settings.


#include "AlternativeRoute.h"
#include "AdaptativeMachine.h"
#include "PlayerMachine.h"

// Sets default values
AAlternativeRoute::AAlternativeRoute()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	MagneticSpline = CreateDefaultSubobject<USplineComponent>(TEXT("SimpleSpline"));
	SetRootComponent(Trigger);
	MagneticSpline->SetupAttachment(Trigger);
}

// Called when the game starts or when spawned
void AAlternativeRoute::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AAlternativeRoute::OnBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AAlternativeRoute::OnEndOverlap);
	
}

// Called every frame
void AAlternativeRoute::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAlternativeRoute::Detour(AActor* OtherActor) {
	AAdaptativeMachine* machine = Cast<AAdaptativeMachine>(OtherActor);

	if (machine) {
		machine->Spline = Spline;
	}
	else {
		APlayerMachine* pMachine = Cast<APlayerMachine>(OtherActor);
		pMachine->Spline = Spline;
	}
}

void AAlternativeRoute::StartMagneticBehavior(AActor* OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->StartMagnetic(MagneticSpline);
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("magnetic")));
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->StartMagnetic(MagneticSpline);
	}
}

void AAlternativeRoute::EndMagneticBehavior(AActor* OtherActor) {
	AAdaptativeMachine* aiMachine = Cast<AAdaptativeMachine>(OtherActor);
	if (aiMachine) {
		aiMachine->EndMagnetic();
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("end magnetic")));
		APlayerMachine* playerMachine = Cast<APlayerMachine>(OtherActor);
		playerMachine->EndMagnetic();
	}
}

void AAlternativeRoute::OnBeginOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (bMagnetic) {
		StartMagneticBehavior(OtherActor);
	}
	else {
		Detour(OtherActor);
	}
}

void AAlternativeRoute::OnEndOverlap(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {
	if (bMagnetic) {
		EndMagneticBehavior(OtherActor);
	}
}

