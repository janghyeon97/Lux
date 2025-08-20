// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainHUD.h"

#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/ActionSystemComponent.h"

#include "UI/VitalsWidget.h"
#include "UI/UW_ActionPanel.h"
#include "UI/UW_ActionTooltip.h"

#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"

#include "LuxLogChannels.h" 
#include "Game/LuxPlayerState.h"

UMainHUD::UMainHUD(const FObjectInitializer& ObjectInitializer) 
    : Super(ObjectInitializer)
{
    ASC = nullptr;
    VitalsWidget = nullptr;
    ActionPanel = nullptr;
    TooltipSizeBox = nullptr;
}

void UMainHUD::NativeConstruct()
{
    Super::NativeConstruct();

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogLux, Warning, TEXT("UMainHUD::NativeConstruct - PlayerController is not valid. UI will not be initialized."));
        return;
    }

    ALuxPlayerState* PS = PC->GetPlayerState<ALuxPlayerState>();
    if (PS)
    {
        ASC = PS->GetActionSystemComponent();
    }
    else
    {
        UE_LOG(LogLux, Warning, TEXT("UMainHUD::NativeConstruct - LuxPlayerState is not valid. UI will not be initialized."));
        return;
    }

    if (!ASC.IsValid())
    {
        UE_LOG(LogLux, Warning, TEXT("UMainHUD::NativeConstruct - ActionSystemComponent is not valid. UI will not be initialized."));
        return;
    }

    if (VitalsWidget)
    {
        VitalsWidget->InitializeWidget(ASC.Get());
    }

    if (ActionPanel)
    {
        ActionPanel->InitializeWidget(ASC.Get());
        ActionPanel->SetMainHUDReference(this);
    }

    // 툴팁 SizeBox 초기화 (초기에는 숨김)
    if (TooltipSizeBox)
    {
        TooltipSizeBox->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogLux, Log, TEXT("[%s] 툴팁 SizeBox가 초기화되었습니다."), *GetNameSafe(this));
    }
    else
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] TooltipSizeBox가 바인딩되지 않았습니다. 블루프린트에서 SizeBox를 추가해주세요."), *GetNameSafe(this));
    }
}

void UMainHUD::NativeDestruct()
{
    Super::NativeDestruct();

}

void UMainHUD::InitializeWidget(UActionSystemComponent* InASC)
{
    Super::InitializeWidget(InASC);

    if(ASC.IsValid() == false)
    {
        UE_LOG(LogLux, Error, TEXT("[%s] ActionSystem 이 유효하지 않습니다."), *GetNameSafe(this));
        return;
    }

    if(VitalsWidget)
    {
        VitalsWidget->InitializeWidget(InASC);
    }   

    if(ActionPanel)
    {
        ActionPanel->InitializeWidget(InASC);
        // ActionIcon들에 MainHUD 참조 설정
        ActionPanel->SetMainHUDReference(this);
    }
}

void UMainHUD::ShowTooltip(UUW_ActionTooltip* TooltipWidget)
{
    if (!TooltipSizeBox || !TooltipWidget)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] ShowTooltip: TooltipSizeBox 또는 TooltipWidget이 유효하지 않습니다."), *GetNameSafe(this));
        return;
    }

    // 기존 콘텐츠 제거
    TooltipSizeBox->ClearChildren();

    // 새 툴팁을 SizeBox에 추가
    USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(TooltipSizeBox->AddChild(TooltipWidget));
    if (SizeBoxSlot)
    {
        SizeBoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
        SizeBoxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
    }

    // SizeBox 표시
    TooltipSizeBox->SetVisibility(ESlateVisibility::Visible);
    
    UE_LOG(LogLux, Warning, TEXT("[%s] 툴팁이 표시되었습니다."), *GetNameSafe(this));
}

void UMainHUD::HideTooltip()
{
    if (!TooltipSizeBox)
    {
        return;
    }

    // SizeBox 숨기기
    TooltipSizeBox->SetVisibility(ESlateVisibility::Collapsed);
    
    // 콘텐츠 제거 (메모리 정리)
    TooltipSizeBox->ClearChildren();
    
    UE_LOG(LogLux, Warning, TEXT("[%s] 툴팁이 숨겨졌습니다."), *GetNameSafe(this));
}
