// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerMachine.h"
#include "ZeroBaseWidget.h"
#include "TrackManager_v2.h"
#include "ZeroHUD.generated.h"

/**
 * 
 */
UCLASS()
class ZEROPROJECT_API AZeroHUD : public AHUD
{
	GENERATED_BODY()

public:
	AZeroHUD();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called when the game starts or when spawned
	virtual void DrawHUD() override;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	APlayerMachine* PlayerMachine;
	FNumberFormattingOptions* TwoDigitsOptions;
	FNumberFormattingOptions* ThreeDigitsOptions;
	float Countdown;
	float Time;
	float BarPercent;
	bool RaceStarted;
	float SpeedMetric;

	UPROPERTY(EditAnywhere)
		TSubclassOf<class UZeroBaseWidget> ZeroBaseWidgetClass;

	UPROPERTY()
		UZeroBaseWidget* RacingUI;

	void OnButtonClicked();

public:
	UPROPERTY(BlueprintReadOnly)
	ATrackManager_v2* TrackManager;

	void ShowPauseMenu();
	void HidePauseMenu();
	void ShowEndRaceScreen();
};
