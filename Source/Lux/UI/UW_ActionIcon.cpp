// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UW_ActionIcon.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "ActionSystem/Cooldown/LuxCooldownTracker.h" 

#include "System/LuxAssetManager.h"
#include "Game/LuxPlayerState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/UW_ActionTooltip.h"
#include "UI/MainHUD.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "LuxLogChannels.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"




void UUW_ActionIcon::InitializeWidget(UActionSystemComponent* InASC)
{
    Super::InitializeWidget(InASC);

    UE_LOG(LogLux, Error, TEXT("[%s] ActionIcon 위젯은 이 함수를 호출해서는 안됩니다. 대신 InitializeWidget(InASC, InSpec)를 사용하세요."), *GetNameSafe(this));
}

void UUW_ActionIcon::InitializeWidget(UActionSystemComponent* InASC, const FLuxActionSpec& InSpec)
{
    // 1) 기본 상태 초기화: 쿨다운 퍼센트 0, 시간 0 및 텍스트 숨김
    if (ImageIcon)
    {
        CooldownMaterialDynamic = ImageIcon->GetDynamicMaterial();
    }

    SetCooldownPercent(0.0f);

    if (CooldownText)
    {
        CooldownText->SetText(FText::FromString(TEXT("0")));
        CooldownText->SetVisibility(ESlateVisibility::Hidden);
    }

    // 2) 필수 레퍼런스/데이터 저장
    ASC = InASC;
    ActionSpecHandle = InSpec.Handle;
    CurrentActionSpec = InSpec;
    CooldownTag = InSpec.GetCooldownTag();

    // 3) 액션 유효성 검사
    if (!InSpec.Action)
    {
        UE_LOG(LogLux, Error, TEXT("[%s] Action이 설정되어 있지 않습니다. 위젯을 초기화하지 않습니다."), *GetNameSafe(this));
        return;
    }

    // 4) 아이콘 로드 및 설정
    ULuxAssetManager& AssetManager = ULuxAssetManager::Get();
    UTexture2D* IconTexture = AssetManager.GetAsset(InSpec.Action->Icon);
    if (IconTexture)
    {
        SetIcon(IconTexture);
    }
    else
    {
        UE_LOG(LogLux, Error, TEXT("[%s] '%s' 액션의 Icon이 설정되어 있지 않습니다."), *GetNameSafe(this), *InSpec.Action->GetName());
        return;
    }

    // 5) 쿨다운 태그 유효성 검사
    if(CooldownTag.IsValid() == false)
    {
        UE_LOG(LogLux, Error, TEXT("[%s] '%s' 액션의 CooldownTag가 설정되어 있지 않습니다. 쿨다운 표시 기능이 비활성화됩니다."), *GetNameSafe(this), *InSpec.Action->GetName());
        return;
    }

    // 6) ASC / 쿨다운 트래커 바인딩
    if (ASC.IsValid())
    {
        CooldownTracker = ASC->GetCooldownTracker();
        // ASC 유효함, CooldownTracker 획득

        if (CooldownTracker.IsValid())
        {
            // CooldownTracker 델리게이트 바인딩
            CooldownTracker->OnCooldownAdded.AddDynamic(this, &UUW_ActionIcon::OnCooldownStarted);
            CooldownTracker->OnCooldownChanged.AddDynamic(this, &UUW_ActionIcon::OnCooldownProgressChanged);
            CooldownTracker->OnCooldownRemoved.AddDynamic(this, &UUW_ActionIcon::OnCooldownEnded);
            
            // 현재 쿨다운 상태 확인
            const float CurrentRemaining = CooldownTracker->GetTimeRemaining(CooldownTag);
            const float CurrentDuration = CooldownTracker->GetDuration(CooldownTag);
        }
        else
        {
            UE_LOG(LogLuxCooldown, Error, TEXT("[%s] CooldownTracker 획득 실패. ASC는 유효하지만 트래커가 null입니다."), *GetNameSafe(this));
        }
    }
    else
    {
        UE_LOG(LogLuxCooldown, Error, TEXT("[%s] ASC가 유효하지 않습니다. 쿨다운 표시 기능이 비활성화됩니다."), *GetNameSafe(this));
        return;
    }

    // 7) 툴팁 위젯 생성 및 초기화
    if (!TooltipWidgetClass)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] TooltipWidgetClass가 설정되지 않았습니다. 툴팁 기능이 비활성화됩니다."), *GetNameSafe(this));
        return;
    }

    CurrentTooltip = CreateWidget<UUW_ActionTooltip>(this, TooltipWidgetClass);
    if (CurrentTooltip)
    {
        CurrentTooltip->InitializeTooltip(ASC.Get(), InSpec);
        CurrentTooltip->UpdateTooltipIcon(IconTexture);
        UE_LOG(LogLux, Log, TEXT("[%s] 툴팁이 성공적으로 생성되고 초기화되었습니다."), *GetNameSafe(this));
    }
    else
    {
        UE_LOG(LogLux, Error, TEXT("[%s] 툴팁 위젯 생성에 실패했습니다."), *GetNameSafe(this));
    }
}

void UUW_ActionIcon::NativeConstruct()
{
    Super::NativeConstruct();

}

void UUW_ActionIcon::NativeDestruct()
{
    // 쿨다운 타이머 정리
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CooldownTimerHandle);
    }

    // CooldownTracker 델리게이트 언바인딩
    if (CooldownTracker.IsValid())
    {
        CooldownTracker->OnCooldownAdded.RemoveDynamic(this, &UUW_ActionIcon::OnCooldownStarted);
        CooldownTracker->OnCooldownChanged.RemoveDynamic(this, &UUW_ActionIcon::OnCooldownProgressChanged);
        CooldownTracker->OnCooldownRemoved.RemoveDynamic(this, &UUW_ActionIcon::OnCooldownEnded);
    }

    // 툴팁 정리
    if (CurrentTooltip)
    {
        CurrentTooltip = nullptr;
    }
    
    // 참조 해제
    MainHUD = nullptr;
    CooldownTracker = nullptr;

    Super::NativeDestruct();
}

void UUW_ActionIcon::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    // 툴팁 표시
    if (MainHUD.IsValid() && CurrentTooltip && ASC.IsValid())
    {
        // 툴팁 텍스트 정보를 강제로 다시 업데이트 (첫 번째 마우스 오버 문제 해결)
        CurrentTooltip->UpdateTooltipData();
        
        // MainHUD를 통해 툴팁 표시
        MainHUD->ShowTooltip(CurrentTooltip);
    }
}

void UUW_ActionIcon::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    // MainHUD를 통해 툴팁 숨기기
    if (MainHUD.IsValid())
    {
        MainHUD->HideTooltip();
    }
}

void UUW_ActionIcon::OnCooldownStarted(const FGameplayTag& InCooldownTag, float Duration)
{
    // OnCooldownStarted 호출됨
    
    if (InCooldownTag != CooldownTag)
    {
        // 태그 불일치는 정상 동작이므로 로그 제거
        return;
    }

    // 쿨다운 시작 처리 중
    
    // 쿨다운 시작 - 텍스트 표시
    if (CooldownText)
    {
        CooldownText->SetVisibility(ESlateVisibility::Visible);
        // CooldownText 표시 설정 완료
    }
    else
    {
        UE_LOG(LogLuxCooldown, Error, TEXT("[%s] CooldownText가 null입니다"), *GetNameSafe(this));
        return;
    }
    
    // 실시간 UI 업데이트를 위한 타이머 시작
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            CooldownTimerHandle,
            this,
            &UUW_ActionIcon::UpdateCooldownUI,
            UpdateInterval,
            true  // 반복
        );
        // 쿨다운 UI 업데이트 타이머 시작
    }
}

void UUW_ActionIcon::OnCooldownProgressChanged(const FGameplayTag& InCooldownTag, float TimeRemaining, float Duration)
{
    if (InCooldownTag != CooldownTag)
    {
        return;
    }

    PlayAnimation(ReductionAnimation, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);

    // 쿨다운 진행률 업데이트
    if (CooldownText)
    {
        FString CooldownTimeString = (TimeRemaining <= 1.0f)
            ? FString::Printf(TEXT("%.1f"), TimeRemaining)
            : FString::Printf(TEXT("%d"), FMath::CeilToInt(TimeRemaining));

        CooldownText->SetText(FText::FromString(CooldownTimeString));
    }
    else
    {
        UE_LOG(LogLuxCooldown, Error, TEXT("[%s] CooldownText가 null입니다"), *GetNameSafe(this));
    }

    // 남은 시간 비율을 계산하여 다이나믹 머티리얼의 파라미터를 업데이트합니다.
    if (CooldownMaterialDynamic && Duration > 0)
    {
        const float Percent = TimeRemaining / Duration;
        const float MaterialPercent = 1.0f - Percent;
        CooldownMaterialDynamic->SetScalarParameterValue(FName("Percent"), MaterialPercent);
        // 머티리얼 퍼센트 업데이트
    }
    else
    {
        UE_LOG(LogLuxCooldown, Error, TEXT("[%s] 머티리얼 업데이트 실패 - CooldownMaterialDynamic: %s, Duration: %f"), 
               *GetNameSafe(this), CooldownMaterialDynamic ? TEXT("유효") : TEXT("null"), Duration);
    }
}

void UUW_ActionIcon::OnCooldownEnded(const FGameplayTag& InCooldownTag)
{
    // OnCooldownEnded 호출됨
    
    if (InCooldownTag != CooldownTag)
    {
        // 태그 불일치는 정상 동작이므로 로그 제거
        return;
    }

    // 쿨다운 종료 처리 중

    // 타이머 정지
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CooldownTimerHandle);
        // 쿨다운 UI 업데이트 타이머 정지
    }

    // 쿨다운 종료 - UI 숨기기 및 퍼센트 0으로 설정
    if (CooldownText)
    {
        CooldownText->SetVisibility(ESlateVisibility::Hidden);
        // CooldownText 숨김 설정 완료
    }
    else
    {
        UE_LOG(LogLuxCooldown, Error, TEXT("[%s] CooldownText가 null입니다"), *GetNameSafe(this));
    }
    
    if (CooldownMaterialDynamic)
    {
        CooldownMaterialDynamic->SetScalarParameterValue(FName("Percent"), 0.0f);
        // 머티리얼 퍼센트 0으로 리셋 완료
    }
    else
    {
        UE_LOG(LogLuxCooldown, Error, TEXT("[%s] CooldownMaterialDynamic이 null입니다"), *GetNameSafe(this));
    }
}

void UUW_ActionIcon::UpdateCooldownUI()
{
    // CooldownTracker에서 현재 쿨다운 상태 가져오기
    if (!CooldownTracker.IsValid() || !CooldownTag.IsValid())
    {
        return;
    }
    
    const float TimeRemaining = CooldownTracker->GetTimeRemaining(CooldownTag);
    const float Duration = CooldownTracker->GetDuration(CooldownTag);
    
    // 쿨다운이 끝났으면 타이머 정지
    if (TimeRemaining <= 0.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(CooldownTimerHandle);
            // 쿨다운 완료로 타이머 자동 정지
        }
        
        // 최종 상태로 UI 업데이트
        if (CooldownText)
        {
            CooldownText->SetVisibility(ESlateVisibility::Hidden);
        }
        if (CooldownMaterialDynamic)
        {
            CooldownMaterialDynamic->SetScalarParameterValue(FName("Percent"), 0.0f);
        }
        return;
    }
    
    // 실시간 UI 업데이트
    if (CooldownText)
    {
        FString CooldownTimeString = (TimeRemaining <= 1.0f)
            ? FString::Printf(TEXT("%.1f"), TimeRemaining)
            : FString::Printf(TEXT("%d"), FMath::CeilToInt(TimeRemaining));
        
        CooldownText->SetText(FText::FromString(CooldownTimeString));
        // 타이머 UI 업데이트
    }
    
    // 머티리얼 퍼센트 업데이트
    if (CooldownMaterialDynamic && Duration > 0)
    {
        const float Percent = TimeRemaining / Duration;
        const float MaterialPercent = 1.0f - Percent;
        CooldownMaterialDynamic->SetScalarParameterValue(FName("Percent"), MaterialPercent);
        // 타이머 머티리얼 업데이트
    }
}

void UUW_ActionIcon::SetIcon(UTexture* NewIcon)
{
    if (::IsValid(ImageIcon) == false)
    {
        UE_LOG(LogLux, Error, TEXT("[%s] ImageIcon 이 설정되어 있지 않습니다."), *GetNameSafe(this));
        return;
    }

    UMaterialInstanceDynamic* DynamicMaterial = ImageIcon->GetDynamicMaterial();
    if (DynamicMaterial)
    {
        DynamicMaterial->SetTextureParameterValue(FName("Texture"), NewIcon);
    }
}

void UUW_ActionIcon::SetCooldownPercent(float Percent)
{
    if (CooldownMaterialDynamic)
    {
        CooldownMaterialDynamic->SetScalarParameterValue(FName("Percent"), Percent);
    }
}

void UUW_ActionIcon::SetMainHUDReference(UMainHUD* InMainHUD)
{
    MainHUD = InMainHUD;
    
    if (MainHUD.IsValid())
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] MainHUD 참조가 설정되었습니다: %s"), *GetNameSafe(this), *GetNameSafe(MainHUD.Get()));
    }
    else
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] MainHUD 참조가 null로 설정되었습니다."), *GetNameSafe(this));
    }
}