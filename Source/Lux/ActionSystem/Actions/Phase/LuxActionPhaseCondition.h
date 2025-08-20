#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/Phase/LuxActionPhaseTypes.h"
#include "LuxActionPhaseCondition.generated.h"

USTRUCT(BlueprintType)
struct FCondition_PathHasMinPoints : public FPhaseConditionBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
    int32 MinPoints = 2;
    virtual bool CheckCondition(const class ULuxAction& Action, const struct FContextPayload& Payload) const override;
};

USTRUCT(BlueprintType)
struct FCondition_NotifyNameEquals : public FPhaseConditionBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
    FName RequiredName;

    virtual bool CheckCondition(const class ULuxAction& Action, const struct FContextPayload& Payload) const override;
}; 