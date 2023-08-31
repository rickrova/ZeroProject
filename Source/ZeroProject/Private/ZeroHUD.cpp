// Fill out your copyright notice in the Description page of Project Settings.


#include "ZeroHUD.h"
#include "Blueprint/UserWidget.h"

AZeroHUD::AZeroHUD()
{
    PrimaryActorTick.bCanEverTick = true;

    Countdown = 3.f;
    Time = 0.f;
    SpeedMetric = 0;
    RaceStarted = false;

    TwoDigitsOptions = new FNumberFormattingOptions();
    ThreeDigitsOptions = new FNumberFormattingOptions();
    TwoDigitsOptions->MinimumIntegralDigits = 2;
    ThreeDigitsOptions->MinimumIntegralDigits = 3;
    ThreeDigitsOptions->MaximumFractionalDigits = 0;
    ThreeDigitsOptions->SetUseGrouping(false);

    PlayerMachine = nullptr;
    TrackManager = nullptr;
    RacingUI = nullptr;
}

void AZeroHUD::BeginPlay()
{
    Super::BeginPlay();

    PlayerMachine = Cast<APlayerMachine>(GetOwningPlayerController()->GetPawn());
    TrackManager = PlayerMachine->TrackManager;
    RacingUI = CreateWidget<UZeroBaseWidget>(GetOwningPlayerController(), ZeroBaseWidgetClass);
    check(RacingUI);
    RacingUI->AddToPlayerScreen();

    TrackManager->HUD = this;
    RacingUI->HUD = this;
    RacingUI->Resume_Button->OnClicked.AddUniqueDynamic(this, &AZeroHUD::OnButtonClicked);
}

void AZeroHUD::OnButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Button Clicked"));
}

void AZeroHUD::DrawHUD() {
    if (!RaceStarted) {
        int countdownText = Countdown + 1;
        RacingUI->Countdown_Text->SetText(FText::AsNumber(countdownText));
    }

    FText speedText = FText::AsNumber(SpeedMetric, ThreeDigitsOptions);
    RacingUI->Speed_Text->SetText(speedText);

    FText rankingText = FText::Format(INVTEXT("{0} / {1}"),
        FText::AsNumber(PlayerMachine->Rank, TwoDigitsOptions),
        FText::AsNumber(TrackManager->ActiveMachines, TwoDigitsOptions));
    RacingUI->Ranking_Text->SetText(rankingText);

    int minutes = Time / 60;
    int seconds = Time - (minutes * 60);
    int miliseconds = (Time - (int)Time) * 1000;
    FText timeText = FText::Format(INVTEXT("{0}' {1}'' {2}"),
        FText::AsNumber(minutes, TwoDigitsOptions),
        FText::AsNumber(seconds, TwoDigitsOptions), 
        FText::AsNumber(miliseconds, ThreeDigitsOptions));
    RacingUI->Time_Text->SetText(timeText);

    RacingUI->Energy_Bar->SetPercent(BarPercent);

    FText lapText = FText::Format(INVTEXT("{0} / {1}"), TrackManager->PlayerCurrentLap, TrackManager->Laps);
    RacingUI->Lap_Text->SetText(lapText);
}

void AZeroHUD::Tick(float DeltaTime)
{
    if (!RaceStarted) {
        Countdown -= DeltaTime;
        if (Countdown <= 0) {
            Countdown = 0;
            RaceStarted = true;
            TrackManager->StartRace();
            RacingUI->Countdown_Text->SetText(INVTEXT("GO!"));
        }
    }
    else {
        Time += DeltaTime;
        if (RacingUI->Countdown_Text->GetVisibility() == ESlateVisibility::Visible && Time > 3) {
            RacingUI->Countdown_Text->SetVisibility(ESlateVisibility::Hidden);
        }

        float speedRaw = PlayerMachine->LastDeltaLocation.Size() * 0.036f;
        SpeedMetric = FMath::Lerp(SpeedMetric, speedRaw, DeltaTime * 10.f);

        BarPercent = FMath::Lerp(RacingUI->Energy_Bar->GetPercent(), PlayerMachine->Condition, DeltaTime * 2.f);
    }
}

void AZeroHUD::ShowPauseMenu() {
    RacingUI->Pause_Panel->SetVisibility(ESlateVisibility::Visible);
    RacingUI->Resume_Button->SetKeyboardFocus();
}

void AZeroHUD::HidePauseMenu() {
    RacingUI->Pause_Panel->SetVisibility(ESlateVisibility::Hidden);
}

void AZeroHUD::ShowEndRaceScreen() {
    FText endPlaceText = FText::Format(INVTEXT("{0}{0}|ordinal(one = st, two = nd, few = rd, other = th)"),
        PlayerMachine->Rank);
    RacingUI->End_Place_Text->SetText(endPlaceText);
    RacingUI->End_Time_Text->SetText(RacingUI->Time_Text->GetText());
    RacingUI->EndRace_Panel->SetVisibility(ESlateVisibility::Visible);
    RacingUI->Retry_Button->SetKeyboardFocus();
}

