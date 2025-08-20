// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "ActionSystem/Actions/Phase/LuxActionPhaseTypes.h"
#include "ActionSystem/Actions/Phase/LuxActionPhaseBehavior.h"
#include "ActionSystem/Actions/Phase/LuxActionPhaseCondition.h"
#include "LuxActionPhaseData.generated.h"

USTRUCT(BlueprintType)
struct FActionPhaseData
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Flow Control")
    TArray<FPhaseTransition> Transitions;

    UPROPERTY(EditAnywhere, Category = "Behaviors", meta = (BaseStruct = "/Script/Lux.PhaseBehaviorBase"))
    TArray<FInstancedStruct> OnEnterBehaviors;

    UPROPERTY(EditAnywhere, Category = "Behaviors", meta = (BaseStruct = "/Script/Lux.PhaseBehaviorBase"))
    TArray<FInstancedStruct> OnExitBehaviors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|State & Effects", meta = (Categories = "Action"))
    FGameplayTagContainer CancelActionsWithTag;
    
    /** 페이즈가 활성화된 동안 플레이어가 움직일 경우 재생 중인 애니메이션 몽타주를 즉시 중단시킬지 여부를 결정합니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Animation")
    bool bCanAnimationBeInterruptedByMovement = false;
};

UCLASS(BlueprintType, Const)
class LUX_API ULuxActionPhaseData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phases")
    TMap<FGameplayTag, FActionPhaseData> Phases;
}; 