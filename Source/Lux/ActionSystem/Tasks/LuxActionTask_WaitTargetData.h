// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "LuxActionTask_WaitTargetData.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTargetDataReadyDelegate, const FHitResult&, HitResult);

class ALuxPlayerController;

/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_WaitTargetData : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
	/** 타겟 지정을 기다리는 Task를 생성합니다. */
	UFUNCTION(BlueprintCallable, Category = "Action|Tasks", meta = (DisplayName = "Wait Target Data"))
	static ULuxActionTask_WaitTargetData* WaitTargetData(ULuxAction* InOwningAction);

protected:
	// ~ULuxActionTask interface
	virtual void OnActivated() override;
	virtual void OnEnded(bool bSuccess) override;
	virtual void OnBeforeReHome() override;
	virtual void OnAfterReHome() override;
	//~End of ULuxActionTask interface

	// PlayerController의 델리게이트에 바인딩될 콜백 함수들입니다.
	UFUNCTION()
	void OnTargetAcquiredCallback(const FHitResult& HitResult);

	UFUNCTION()
	void OnTargetingCancelledCallback();

protected:
	TWeakObjectPtr<ALuxPlayerController> PlayerController;
};
