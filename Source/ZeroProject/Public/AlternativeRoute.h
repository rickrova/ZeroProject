// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineActor.h"
#include "Components/BoxComponent.h"
#include "AlternativeRoute.generated.h"

UCLASS()
class ZEROPROJECT_API AAlternativeRoute : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAlternativeRoute();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere)
		bool bMagnetic;
	UPROPERTY(VisibleAnywhere)
		UBoxComponent* Trigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		ASplineActor* Spline;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USplineComponent* MagneticSpline;

	void Detour(AActor*);	
	void StartMagneticBehavior(AActor*);
	void EndMagneticBehavior(AActor*);

	UFUNCTION()
		void OnBeginOverlap(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnEndOverlap(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
