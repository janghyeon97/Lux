// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystem/LuxActionSystemTypes.h"
#include "LuxActionTask_WaitGameplayEvent.generated.h"


/**
 * ULuxActionTask_WaitGameplayEvent Task를 위한 초기화 파라미터입니다.
 */
USTRUCT(BlueprintType)
struct FWaitGameplayEventParams : public FBaseLuxActionTaskParams
{
	GENERATED_BODY()

public:
	/** Task가 기다려야 할 전역 게임플레이 이벤트의 태그입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaitGameplayEvent")
	FGameplayTag EventTagToWait;
};


/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_WaitGameplayEvent : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
	/**
	 * 특정 게임플레이 이벤트를 기다리는 Task를 생성합니다.
	 * @param EventTag 기다릴 이벤트의 태그입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Wait GameplayEvent"))
	static ULuxActionTask_WaitGameplayEvent* WaitGameplayEvent(
		ULuxAction* InOwningAction,
		FGameplayTag EventTag
	);

protected:
	// ~ULuxActionTask interface
	virtual void InitializeFromStruct(const FInstancedStruct& Struct) override;
	virtual void OnActivated() override;
	virtual void OnEnded(bool bSuccess) override;
	virtual void OnBeforeReHome() override;
	virtual void OnAfterReHome() override;
	//~End of ULuxActionTask interface

protected:
	UFUNCTION()
	void OnGameplayEventCallback(const FGameplayTag& EventTag, const FContextPayload& Payload);

	UPROPERTY()
	FGameplayTag TagToWaitFor;
};
