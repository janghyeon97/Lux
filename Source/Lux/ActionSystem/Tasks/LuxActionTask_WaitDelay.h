// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "ActionSystem/Actions/LuxDynamicValue.h"
#include "LuxActionTask_WaitDelay.generated.h"


/**
 * ULuxActionTask_WaitDelay Task를 위한 초기화 파라미터입니다.
 */
USTRUCT(BlueprintType)
struct FWaitDelayParams : public FBaseLuxActionTaskParams
{
	GENERATED_BODY()

public:
	/** Task가 기다려야 할 시간 (초)입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaitDelay")
	FDynamicFloat Duration;
};


/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_WaitDelay : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
	/**
	 * 지정된 시간만큼 대기하는 Task를 생성합니다.
	 * @param WaitDuration 대기할 시간 (초 단위) 입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Wait Delay"))
	static ULuxActionTask_WaitDelay* WaitDelay(
		ULuxAction* InOwningAction,
		float WaitDuration
	);

protected:
	// ~ULuxActionTask interface
	virtual void InitializeFromStruct(const FInstancedStruct& Struct) override;
	virtual void OnActivated() override;
	    virtual void OnEnded(bool bSuccess) override;
	//~End of ULuxActionTask interface

	/** 타이머가 만료되었을 때 호출될 콜백 함수입니다. */
	UFUNCTION()
	void OnTimeExpired();

	/** 대기할 시간입니다. */
	float WaitDuration;

	/** 이 Task가 사용하는 타이머의 핸들입니다. */
	FTimerHandle TimerHandle;
};
