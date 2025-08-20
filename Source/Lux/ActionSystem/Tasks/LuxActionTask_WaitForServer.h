// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "LuxActionTask_WaitForServer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnServerActionActivated);

/**
 * 서버 액션이 활성화될 때까지 기다리는 태스크입니다.
 * 클라이언트에서만 사용되며, 서버 액션이 활성화되면 콜백을 실행합니다.
 */
UCLASS()
class LUX_API ULuxActionTask_WaitForServer : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
	ULuxActionTask_WaitForServer();

	/** 서버 액션이 활성화될 때까지 기다리는 태스크를 생성합니다. */
	UFUNCTION(BlueprintCallable, Category = "ActionSystem|Tasks")
	static ULuxActionTask_WaitForServer* WaitForServer(ULuxAction* InOwningAction, float CheckInterval);

	/** 서버 액션이 활성화되었을 때 호출되는 이벤트입니다. */
	UPROPERTY(BlueprintAssignable, Category = "ActionSystem|Tasks")
	FOnServerActionActivated OnServerActionActive;

protected:
	virtual void OnActivated() override;
	virtual void OnEnded(bool bWasCancelled) override;

	/** 서버 액션 상태를 확인하는 함수입니다. */
	void CheckServerActionStatus();

private:
	/** 서버 액션 상태를 주기적으로 확인하는 간격(초)입니다. */
	float CheckIntervalSeconds;

	/** 서버 액션 상태를 주기적으로 확인하는 타이머 핸들입니다. */
	FTimerHandle CheckTimerHandle;

	/** 최대 대기 시간(초)입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "WaitForServer")
	float MaxWaitTime = 5.0f;

	/** 경과 시간을 추적합니다. */
	float ElapsedTime;

	int32* ActiveCountPtr;
};
