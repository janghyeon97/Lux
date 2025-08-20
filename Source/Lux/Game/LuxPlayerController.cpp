// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/LuxPlayerController.h"
#include "Character/LuxPawnExtensionComponent.h"
#include "UI/MainHUD.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

ALuxPlayerController::ALuxPlayerController()
{
    SetActorTickEnabled(false);
    bIsTargeting = false;
}

void ALuxPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어 컨트롤러일 때만 UI를 생성합니다.
    if (IsLocalController())
    {
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);

        bShowMouseCursor = false;
    }
}

void ALuxPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

}

void ALuxPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
}

void ALuxPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

   
}

void ALuxPlayerController::OnUnPossess()
{
    Super::OnUnPossess();

}

void ALuxPlayerController::AcknowledgePossession(APawn* PossessedPawn)
{
    Super::AcknowledgePossession(PossessedPawn);

    if(!PossessedPawn)
    {
        UE_LOG(LogLux, Error, TEXT("[%s] 빙의된 Pawn이 없습니다."), *GetNameSafe(this));
        return;
    }

    auto* PawnExtComponent = ULuxPawnExtensionComponent::FindPawnExtensionComponent(PossessedPawn);
    if(!PawnExtComponent)
    {
        UE_LOG(LogLux, Error, TEXT("[%s] 빙의된 Pawn이 PawnExtensionComponent를 가지고 있지 않습니다."), *GetNameSafe(this));
        return;
    }

    PawnExtComponent->OnActionSystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnActionSystemInitialized));
    PawnExtComponent->OnActionSystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnActionSystemUninitialized));
}

/* ======================================== Action System ======================================== */

void ALuxPlayerController::OnActionSystemInitialized()
{
    UE_LOG(LogLux, Log, TEXT("[%s] 액션시스템이 초기화되었습니다."), *GetNameSafe(this));

    ShowMainHUDWidget();
}

void ALuxPlayerController::OnActionSystemUninitialized()
{
    UE_LOG(LogLux, Log, TEXT("[%s] 액션시스템이 해제되었습니다."), *GetNameSafe(this));

    HideMainHUDWidget();
}

/* ======================================== UI / HUD ======================================== */

void ALuxPlayerController::Client_ShowDamageNumber_Implementation(float DamageAmount, AActor* TargetActor)
{
    if (GEngine && TargetActor)
    {
        const FString DamageText = FString::Printf(TEXT("%s Received %.1f Damage"), *TargetActor->GetName(), DamageAmount);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, DamageText);
    }
}

void ALuxPlayerController::Client_ShowNotification_Implementation(const FText& Message)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, Message.ToString());
    }
}

void ALuxPlayerController::ToggleActionSystemDebug()
{
    bShowActionSystemDebug = !bShowActionSystemDebug;
}

void ALuxPlayerController::ShowMainHUDWidget()
{
    if (!IsLocalController()) return;

    if (!MainHUDWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MainHUDWidgetClass is not set"));
        return;
    }

    if (!MainHUDWidget)
    {
        MainHUDWidget = CreateWidget<UUserWidget>(this, MainHUDWidgetClass);
    }

    if (MainHUDWidget)
    {
        MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
        MainHUDWidget->AddToViewport();
    }
}

void ALuxPlayerController::HideMainHUDWidget()
{
    if (!IsLocalController()) return;

    if (MainHUDWidget)
    {
        MainHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

/* ======================================== Targeting ======================================== */

void ALuxPlayerController::BeginTargeting()
{
    bIsTargeting = true;
    bShowMouseCursor = true;
}

void ALuxPlayerController::EndTargeting()
{
    bIsTargeting = false;
    bShowMouseCursor = false;
}

void ALuxPlayerController::OnTargetingConfirm()
{
    if (!bIsTargeting) return;

    FHitResult HitResult;

    GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
    OnTargetAcquired.Broadcast(HitResult);
    EndTargeting();
}

void ALuxPlayerController::OnTargetingCancel()
{
    if (!bIsTargeting) return;

    OnTargetingCancelled.Broadcast();
    EndTargeting();
}
