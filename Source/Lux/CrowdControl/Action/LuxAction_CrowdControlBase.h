#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "LuxAction_CrowdControlBase.generated.h"



/**
 * CC 액션들의 기본 클래스
 * CC 액션은 캐릭터의 행동을 강제로 제한하는 액션들
 */
UCLASS(BlueprintType)
class LUX_API ULuxAction_CrowdControlBase : public ULuxAction
{
	GENERATED_BODY()

public:
    ULuxAction_CrowdControlBase();

    // 각 CC 액션 블루프린트나 자식 클래스에서 이 태그를 지정해야 합니다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CrowdControl")
    FGameplayTag AssociatedStateTag;

protected:
    // ~ ULuxAction interface
    virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC) override;
    virtual void OnActionEnd(bool bIsCancelled) override;
    //~ End of ULuxAction interface

    // 상태 태그가 제거되었을 때 호출될 콜백 함수
    UFUNCTION()
    void OnStateTagRemoved(const FGameplayTag& Tag, int32 OldCount, int32 NewCount);
}; 