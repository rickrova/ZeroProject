// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "ZeroBaseWidget.generated.h"

/**
 * 
 */

class ATrackManager;	// forward declaration
class AZeroHUD;

UCLASS()
class ZEROPROJECT_API UZeroBaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ATrackManager* TrackManager;

	UPROPERTY(BlueprintReadOnly)
	AZeroHUD* HUD;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* Speed_Text;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* Countdown_Text;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* Ranking_Text;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* Time_Text;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* Lap_Text;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* End_Place_Text;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* End_Time_Text;
	UPROPERTY(meta = (BindWidget))
		UProgressBar* Energy_Bar;
	UPROPERTY(meta = (BindWidget))
		UButton* Resume_Button;
	UPROPERTY(meta = (BindWidget))
		UButton* Restart_Button;
	UPROPERTY(meta = (BindWidget))
		UButton* Retry_Button;
	UPROPERTY(meta = (BindWidget))
		UBorder* Pause_Panel;
	UPROPERTY(meta = (BindWidget))
		UBorder* EndRace_Panel;
};
