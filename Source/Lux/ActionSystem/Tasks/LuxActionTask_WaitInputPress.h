// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "LuxActionTask_WaitInputPress.generated.h"



/**
 * ULuxActionTask_WaitInputPress Task를 위한 초기화 파라미터입니다.
 */
USTRUCT(BlueprintType)
struct FWaitInputPressParams : public FBaseLuxActionTaskParams
{
    GENERATED_BODY()

public:
    /** 이 Task가 기다려야 할 입력 태그입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaitInput", meta = (Categories = "InputTag"))
    FGameplayTag InputTagToWait;
};


/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_WaitInputPress : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
    /** '입력 떼짐'을 기다리는 태스크를 생성합니다. */
    UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Wait Input Release"))
    static ULuxActionTask_WaitInputPress* WaitInputPress(ULuxAction* InOwningAction, FGameplayTag InTagToWaitFor);

protected:
    // ~ULuxActionTask interface
    virtual void InitializeFromStruct(const FInstancedStruct& Struct) override;
    virtual void OnActivated() override;
    virtual void OnEnded(bool bSuccess) override;
    virtual void OnBeforeReHome() override;
    virtual void OnAfterReHome() override;
    //~End of ULuxActionTask interface

    UFUNCTION()
    void OnInputPress(const FGameplayTag& InputTag);

protected:
    /* Task가 기다리고 있는 특정 입력 태그입니다. */
    UPROPERTY()
    FGameplayTag TagToWaitFor;

    /* 입력 바인딩을 위한 핸들 */
    FInputActionBinding* PressBinding;

    /* 중복 호출을 막기 위한 플래그 */
    bool bWasPressed = false;
};
