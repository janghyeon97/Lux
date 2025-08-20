// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"

#include "LuxEffectTypes.h"
#include "LuxEffect.generated.h"


class UActionSystemComponent;
class ULuxExecutionCalculation;

/**
 * 
 */
UCLASS(Blueprintable)
class LUX_API ULuxEffect : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
    ELuxEffectDurationPolicy DurationPolicy;

   // '지속 기간' 정책일 때만 유효한 값들입니다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (EditCondition = "DurationPolicy == ELuxEffectDurationPolicy::HasDuration", EditConditionHides))
    FLuxScalableFloat Duration;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (EditCondition = "DurationPolicy == ELuxEffectDurationPolicy::HasDuration", EditConditionHides))
    FLuxScalableFloat Period;

    // --- 기본 Modifier ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
    TArray<FAttributeModifier> Modifiers;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Calculation")
    TArray<TSubclassOf<ULuxExecutionCalculation>> Executions;

    // --- Stacking ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Stacking")
    EEffectStackingType StackingType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Stacking")
    int32 MaxStacks;

    /** Effect 종류를 정의하는 태그입니다. (예: "State.CrowdControl.Root") */
    UPROPERTY(EditDefaultsOnly, Category = "Effect|Tags")
    FGameplayTagContainer EffectTags;

    /** 이 효과가 적용되는 동안 대상에게 부여할 태그들입니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Tags")
    FGameplayTagContainer GrantedTags;

    /** 이 효과가 적용되기 위해, 대상이 반드시 가지고 있어야 하는 태그들입니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Tags")
    FGameplayTagContainer ApplicationRequiredTags;

    /** 이 효과가 적용되는 것을 막는 태그들입니다. 대상이 이 중 하나라도 가지고 있으면 효과가 적용되지 않습니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Tags")
    FGameplayTagContainer ApplicationBlockedTags;

    /** 이 효과가 적용될 때 대상에게서 제거할 다른 효과들을 태그로 지정합니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Tags")
    FGameplayTagContainer RemoveEffectsWithTags;

    /** 이 효과가 적용될 때 일치하는 태그를 가진 모든 액션을 취소시킵니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|Tags", meta = (Categories = "Action"))
    FGameplayTagContainer CancelActionsWithTags;

    // --- UI 데이터 ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|UI")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|UI", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect|UI")
    TSoftObjectPtr<UTexture2D> Icon;
};