// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "ActionSystem/Actions/LuxDynamicValue.h"
#include "LuxActionTask_FollowSpline.generated.h"


class USplineComponent;
class ACharacter;

/**
 * FollowSpline Task를 위한 초기화 파라미터 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FFollowSplineParams : public FBaseLuxActionTaskParams
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FollowSpline")
    FDynamicFloat Duration;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FollowSpline")
    bool bShouldLockRotation = true;
};

/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_FollowSpline : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
    /**
     * 스플라인을 따라 이동하는 태스크를 생성하고 활성화합니다.
     * @param InOwningAction 이 태스크를 소유하는 액션입니다.
     * @param SplineToFollow 캐릭터가 따라갈 USplineComponent 입니다.
     * @param Duration 이동에 걸리는 총 시간입니다.
     * @param bShouldLockRotation 이동하는 동안 캐릭터의 회전을 스플라인의 방향에 맞춰 잠글지 여부입니다.
     */
    UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Follow Spline"))
    static ULuxActionTask_FollowSpline* FollowSpline(
        ULuxAction* InOwningAction,
        USplineComponent* SplineToFollow,
        float Duration
    );

protected:
    virtual void OnActivated() override;
    virtual void OnEnded(bool bSuccess) override;

private:
    void TickTask();

    UPROPERTY()
    TWeakObjectPtr<USplineComponent> SplineToFollowPtr;

    UPROPERTY()
    float FollowDuration;

    UPROPERTY()
    bool bLockRotation;

    UPROPERTY()
    float ElapsedTime;

    UPROPERTY()
    FVector PreviousVelocity;

    UPROPERTY()
    TEnumAsByte<EMovementMode> OriginalMovementMode;

    FTimerHandle TimerHandle;
};
