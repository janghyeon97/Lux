// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UW_ActionPanel.h"
#include "UI/UW_ActionIcon.h"
#include "UI/MainHUD.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Actions/LuxAction.h"

#include "Components/Border.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"


void UUW_ActionPanel::NativeConstruct()
{
    Super::NativeConstruct();
}

void UUW_ActionPanel::NativeDestruct()
{
    Super::NativeDestruct();

}

void UUW_ActionPanel::InitializeWidget(UActionSystemComponent* InASC)
{
    Super::InitializeWidget(InASC); // 부모 함수 호출로 ASC 저장

    if (ASC.IsValid())
    {
        // 초기 스킬 아이콘 목록을 생성합니다.
        RefreshActionIcons();

        // (선택사항) 나중에 액션이 추가/제거될 때도 UI가 반응하게 하려면
        // ASC에 델리게이트를 만들고 여기에 바인딩하면 됩니다.
        // ASC->OnActionGranted.AddDynamic(this, &UUW_ActionPanel::HandleActionChanged);
    }
}

void UUW_ActionPanel::RefreshActionIcons()
{
    if (!ASC.IsValid() || !ActionIconWidgetClass)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] ASC 혹은 ActionIconWidgetClass가 유효하지 않습니다."), *GetNameSafe(this));
        return;
    }

    // 기존에 추가된 아이콘들이 있다면 모두 제거합니다.
    if(PrimaryActionSlot)       PrimaryActionSlot->ClearChildren();
    if(SecondaryActionSlot)     SecondaryActionSlot->ClearChildren();
    if(TertiaryActionSlot)      TertiaryActionSlot->ClearChildren();
    if(QuaternaryActionSlot)    QuaternaryActionSlot->ClearChildren();
    if(UltimateActionSlot)      UltimateActionSlot->ClearChildren();

    // ActionSystemComponent에서 현재 부여된 모든 액션 Spec 목록을 가져옵니다.
    const TArray<FLuxActionSpec>& ActionSpecs = ASC->GetActionSpecs();

    for (const FLuxActionSpec& Spec : ActionSpecs)
    {
        if (!Spec.Action || !Spec.InputTag.IsValid())
        {
            UE_LOG(LogLux, Warning, TEXT("[%s] 액션이 없거나 입력 태그가 지정되지 않은 Spec입니다."), *GetNameSafe(this));
            continue; // 액션이 없거나 입력 태그가 지정되지 않은 Spec은 건너뜁니다.
        }

        UBorder* TargetSlot = nullptr;

        // InputTag를 확인하여 어떤 슬롯에 배치할지 결정합니다.
        if (Spec.InputTag == LuxGameplayTags::InputTag_Action_Primary)
        {
            TargetSlot = PrimaryActionSlot;
        }
        else if (Spec.InputTag == LuxGameplayTags::InputTag_Action_Secondary)
        {
            TargetSlot = SecondaryActionSlot;
        }
        else if (Spec.InputTag == LuxGameplayTags::InputTag_Action_Tertiary)
        {
            TargetSlot = TertiaryActionSlot;
        }
        else if (Spec.InputTag == LuxGameplayTags::InputTag_Action_Quaternary)
        {
            TargetSlot = QuaternaryActionSlot;
        }
        else if (Spec.InputTag == LuxGameplayTags::InputTag_Action_Ultimate)
        {
            TargetSlot = UltimateActionSlot;
        }

        // 적절한 슬롯을 찾았다면, 아이콘 위젯을 생성하고 추가합니다.
        if (TargetSlot)
        {
            UUW_ActionIcon* NewIconWidget = CreateWidget<UUW_ActionIcon>(this, ActionIconWidgetClass);
            if (NewIconWidget)
            {
                UE_LOG(LogLux, Log, TEXT("[%s] 액션 슬롯 설정: %s"), *GetNameSafe(this), *Spec.InputTag.ToString());

                NewIconWidget->InitializeWidget(ASC.Get(), Spec);
                TargetSlot->AddChild(NewIconWidget);
                ActionIcons.Add(NewIconWidget);
            }
        }
    }
}

void UUW_ActionPanel::SetMainHUDReference(UMainHUD* InMainHUD)
{
    if (!InMainHUD)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] SetMainHUDReference: MainHUD가 유효하지 않습니다."), *GetNameSafe(this));
        return;
    }

    // 모든 ActionIcon에 MainHUD 참조 설정
    for (UUW_ActionIcon* ActionIcon : ActionIcons)
    {
        if (ActionIcon)
        {
            ActionIcon->SetMainHUDReference(InMainHUD);
        }
    }

    UE_LOG(LogLux, Log, TEXT("[%s] %d개의 ActionIcon에 MainHUD 참조를 설정했습니다."), *GetNameSafe(this), ActionIcons.Num());
}