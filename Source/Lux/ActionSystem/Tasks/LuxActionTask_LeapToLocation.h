// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "LuxActionTask_LeapToLocation.generated.h"

/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_LeapToLocation : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
    /**
     * 지정된 위치로 포물선 이동을 수행하는 태스크를 생성합니다.
     * @param InOwningAction 이 태스크를 소유하는 액션입니다.
     * @param Destination 목표 월드 위치입니다.
     * @param Duration 이동에 걸리는 총 시간입니다.
     * @param ArcHeight 포물선의 최대 높이입니다.
     */
    UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task")
    static ULuxActionTask_LeapToLocation* LeapToLocation(
        ULuxAction* InOwningAction,
        FVector Destination,
        float Duration,
        float ArcHeight
    );

protected:
    // ~ULuxActionTask interface
    virtual void OnActivated() override;
    virtual void OnEnded(bool bSuccess) override;
    //~End of ULuxActionTask interface

private:
    // 매 틱 호출되어 캐릭터를 이동시키는 함수
    void TickTask();

    UPROPERTY()
    FVector StartLocation;

    UPROPERTY()
    FVector TargetLocation;

    UPROPERTY()
    float LeapDuration;

    UPROPERTY()
    float LeapArcHeight;

    UPROPERTY()
    float ElapsedTime;

    UPROPERTY()
    FVector PreviousLocation;

    UPROPERTY()
    TEnumAsByte<EMovementMode> OriginalMovementMode;

    FTimerHandle TimerHandle;
};
