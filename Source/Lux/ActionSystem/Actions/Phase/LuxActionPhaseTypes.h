#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "UObject/ObjectMacros.h"
#include "ActionSystem/Actions/LuxDynamicValue.h"
#include "LuxActionPhaseTypes.generated.h"



UENUM(BlueprintType)
enum class EPhaseTransitionType : uint8
{
    /* 즉시 페이즈를 전환합니다. */
    Immediate,

	/* 지정된 시간 후에 페이즈를 전환합니다. */
    OnDurationEnd,

	/* 지정된 GameplayEvent가 발생하면 페이즈를 전환합니다. */
    OnGameplayEvent,

	/* 지정된 TaskEvent가 발생하면 페이즈를 전환합니다. */
    OnTaskEvent,

	/* 수동으로 페이즈를 전환합니다. */
    Manual
};

UENUM(BlueprintType)
enum class EPhaseBehaviorNetExecutionPolicy : uint8
{
	/* 서버에서만 실행됩니다. 클라이언트에서는 실행되지 않습니다. */
    ServerOnly,

	/* 클라이언트에서만 실행됩니다. 서버에서는 실행되지 않습니다. */
    ClientOnly,

	/* 서버와 클라이언트 모두에서 실행됩니다. */
    All,
};


USTRUCT(BlueprintType)
struct FPhaseTransition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    EPhaseTransitionType TransitionType = EPhaseTransitionType::Manual;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (EditCondition = "TransitionType == EPhaseTransitionType::OnDurationEnd", EditConditionHides))
    float Duration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (EditCondition = "TransitionType == EPhaseTransitionType::OnGameplayEvent || TransitionType == EPhaseTransitionType::OnTaskEvent", EditConditionHides))
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (EditCondition = "TransitionType == EPhaseTransitionType::OnTaskEvent", EditConditionHides, BaseStruct = "/Script/Lux.PhaseConditionBase"))
    TArray<FInstancedStruct> Conditions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (Categories = "Phase"))
    FGameplayTag NextPhaseTag;
};


USTRUCT(BlueprintType)
struct FPhaseTaskDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<class ULuxActionTask> TaskClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/Lux.BaseLuxActionTaskParams", ExcludeBaseStruct))
    FInstancedStruct InitializationParameters;
};



USTRUCT(BlueprintType)
struct FPhaseConditionBase
{
    GENERATED_BODY()

    virtual ~FPhaseConditionBase() {}
    virtual bool CheckCondition(const class ULuxAction& Action, const struct FContextPayload& Payload) const { return true; }
}; 