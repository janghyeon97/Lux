// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/AttributeBarBase.h"

#include "ActionSystem/ActionSystemComponent.h"

#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

#include "LuxLogChannels.h"



UAttributeBarBase::UAttributeBarBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    CurrentValue = 0.0f;
    MaxValue = 0.0f;
    RegenerationValue = 0.0f;
}

void UAttributeBarBase::NativeConstruct()
{
	Super::NativeConstruct();
}

void UAttributeBarBase::NativeDestruct()
{
	Super::NativeDestruct();

    // 델리게이트 프로퍼티와 소유자 객체가 모두 유효한지 확인합니다.
    for (FBoundDelegateInfo& BoundDelegate : BoundDelegates)
    {
        if (BoundDelegate.DelegateProperty && BoundDelegate.OwningObject.IsValid())
        {
            BoundDelegate.DelegateProperty->RemoveDelegate(BoundDelegate.Delegate, BoundDelegate.OwningObject.Get());
        }
    }

    BoundDelegates.Empty();
}

void UAttributeBarBase::InitializeWidget(UActionSystemComponent* InASC)
{
	Super::InitializeWidget(InASC);

	if (!InASC)
	{
		UE_LOG(LogLux, Error, TEXT("UAttributeBarBase::InitializeWidget: ActionSystemComponent is not valid"));
		return;
	}

    // ASC에서 AvatarActor를 Owner로 사용
    AActor* Owner = InASC->GetAvatarActor();
    if (!Owner)
    {
        UE_LOG(LogLux, Error, TEXT("[%s][%s]: ASC에서 AvatarActor를 찾을 수 없음"), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }
    
    UE_LOG(LogLux, Log, TEXT("[%s][%s][%s]: ASC의 AvatarActor '%s'를 Owner로 사용"), *Owner->GetName(), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *Owner->GetName());

    if(AttributeToTrack.IsValid())
    {
        CurrentValue = InitializeAttribute(AttributeToTrack, GET_FUNCTION_NAME_CHECKED(ThisClass, OnAttributeChanged));
    }
    else
    {
        UE_LOG(LogLux, Error, TEXT("[%s][%s][%s]: 현재값을 초기화할 Attribute가 유효하지 않습니다."), *Owner->GetName(), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
    }

    if(MaxAttributeToTrack.IsValid())
    {
        MaxValue = InitializeAttribute(MaxAttributeToTrack, GET_FUNCTION_NAME_CHECKED(ThisClass, OnMaxAttributeChanged));
    }
    else
    {
        UE_LOG(LogLux, Error, TEXT("[%s][%s][%s]: 최대값을 초기화할 Attribute가 유효하지 않습니다."), *Owner->GetName(), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
    }

	if (RegenerationAttributeToTrack.IsValid())
	{
		RegenerationValue = InitializeAttribute(RegenerationAttributeToTrack, GET_FUNCTION_NAME_CHECKED(ThisClass, OnRegenerationAttributeChanged));
    }
    else
    {
        UE_LOG(LogLux, Error, TEXT("[%s][%s][%s]: 재생량을 초기화할 Attribute가 유효하지 않습니다."), *Owner->GetName(), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
	}

	// 초기 가시성 설정
	UpdateBar();
}

float UAttributeBarBase::InitializeAttribute(FLuxAttribute& AttributeToInitialize, FName BoundFunctionName)
{
    if (ASC.IsValid() == false)
    {
        UE_LOG(LogLux, Error, TEXT("[%s][%s] ASC 가 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
        return 0.0f;
    }

    FString OwnerName = ASC->GetAvatarActor() ? ASC->GetAvatarActor()->GetName() : TEXT("NULL");

    if (!AttributeToInitialize.IsValid())
    {
        UE_LOG(LogLux, Error, TEXT("[%s][%s][%s] Attribute 가 설정되어 있지 않습니다."), *OwnerName, *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
        return 0.0f;
    }

    ULuxAttributeSet* FoundSet = ASC->GetAttributeSet(AttributeToInitialize.GetAttributeSetClass());
	if (!FoundSet)
	{
		UE_LOG(LogLux, Error, TEXT("[%s][%s][%s] 액션시스템 '%s'에 추적하는 '%s' AttributeSet이 없습니다."), *OwnerName, *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *ASC->GetName(), *AttributeToInitialize.GetName().ToString());
		return 0.0f;
	}

    FString DelegateName = FString::Printf(TEXT("On%sChanged"), *AttributeToInitialize.GetName().ToString());
	FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(FoundSet->GetClass(), FName(*DelegateName));
	if (!DelegateProperty)
	{
		UE_LOG(LogLux, Error, TEXT("[%s][%s][%s] 액션시스템 '%s'에 추적하는 '%s' AttributeSet의 '%s' 델리게이트를 찾을 수 없습니다."), *OwnerName, *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *ASC->GetName(), *AttributeToInitialize.GetName().ToString(), *DelegateName);
		return 0.0f;
	}

    FScriptDelegate NewDelegate;
	NewDelegate.BindUFunction(this, BoundFunctionName);	
	DelegateProperty->AddDelegate(NewDelegate, FoundSet);

    BoundDelegates.Add({ DelegateProperty, NewDelegate, FoundSet });
    UE_LOG(LogLux, Log, TEXT("[%s][%s][%s] 액션시스템 '%s'에 추적하는 '%s' AttributeSet의 '%s' 델리게이트에 바인딩 성공"), *OwnerName, *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *ASC->GetName(), *AttributeToInitialize.GetName().ToString(), *DelegateName);

    FLuxAttributeData* LuxAttributeData = AttributeToInitialize.GetAttributeDataChecked(FoundSet);
    if(LuxAttributeData)
    {
        return LuxAttributeData->GetCurrentValue();
    }

    return 0.0f;
}

void UAttributeBarBase::OnAttributeChanged(float OldValue, float NewValue)
{
    CurrentValue = NewValue;
    UpdateBar();
}

void UAttributeBarBase::OnMaxAttributeChanged(float OldValue, float NewValue)
{
    MaxValue = NewValue;
    UpdateBar();
}

void UAttributeBarBase::OnRegenerationAttributeChanged(float OldValue, float NewValue)
{
    RegenerationValue = NewValue;
    UpdateBar();
}

void UAttributeBarBase::UpdateBar()
{
    // ProgressBar 유효성 검사
    if (!IsValid(ProgressBar))
    {
        UE_LOG(LogLux, Error, TEXT("[%s] ProgressBar가 유효하지 않습니다."), *GetNameSafe(this));
        return;
    }

    // 기본 Attribute들의 유효성에 따라 전체 UI 가시성 결정합니다.
    const bool bShouldShowMainUI = AttributeToTrack.IsValid() && MaxAttributeToTrack.IsValid();
    
    // ProgressBar와 ValueText 가시성 설정합니다.
    ProgressBar->SetVisibility(bShouldShowMainUI ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (ValueText)
    {
        ValueText->SetVisibility(bShouldShowMainUI ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    // Regeneration UI 가시성 설정합니다.
    if (RegenerationText)
    {
        const bool bShouldShowRegeneration = RegenerationAttributeToTrack.IsValid();
        RegenerationText->SetVisibility(bShouldShowRegeneration ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    // UI가 숨겨져 있으면 값 업데이트를 건너뜁니다.
    if (!bShouldShowMainUI)
    {
        return;
    }

    const float Percent = (MaxValue > 0) ? (CurrentValue / MaxValue) : 0.0f;
    ProgressBar->SetPercent(Percent);

    if (ValueText)
    {
        const FString TextString = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(CurrentValue), FMath::CeilToInt(MaxValue));
        ValueText->SetText(FText::FromString(TextString));
    }

    if (RegenerationAttributeToTrack.IsValid() && RegenerationText)
    {
        const FString RegenerationString = FString::Printf(TEXT("+%.1f/s"), RegenerationValue);
        RegenerationText->SetText(FText::FromString(RegenerationString));
    }
}