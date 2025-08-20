// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "LuxActionTask_WaitPhaseDelay.generated.h"

/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_WaitPhaseDelay : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Wait Phase Delay"))
    static ULuxActionTask_WaitPhaseDelay* WaitPhaseDelay(
        ULuxAction* InOwningAction,
        float WaitDuration    
    );

protected:
    // ~ULuxActionTask interface
    virtual void OnActivated() override;
    virtual void OnEnded(bool bSuccess) override;
    //~End of ULuxActionTask interface

    UFUNCTION()
    void OnTimeExpired();

    float Duration;
    FTimerHandle TimerHandle;
};
