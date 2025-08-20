// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ActionSystem/Actions/LuxActionTypes.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "LuxActionTask.generated.h"

class ULuxAction;
class UActionSystemComponent;


UENUM()
enum class ELuxTaskLifecycleState : uint8
{
    /** 아직 활성화되지 않은 상태. */
    Inactive,

    /** 활성화되어 실행 중인 상태.*/
    Executing,

    /** 종료 절차 진행 중 */
    Ending,

    /** 모든 정리가 끝나고 완전히 종료된 상태. */
    Ended
};

USTRUCT(BlueprintType)
struct FBaseLuxActionTaskParams
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName TaskName;
};


UCLASS()
class LUX_API ULuxActionTask : public UObject
{
	GENERATED_BODY()

    friend class ULuxAction;

public:
	ULuxActionTask();

    /** 태스크가 활성화될 때 호출됩니다. */
    void Activate();

    /** Task가 종료될 때 호출됩니다. 
     * @param bSuccess - 태스크의 성공 여부
     */
    void EndTask(bool bSuccess);

    /** 현재 태스크의 라이프사이클 상태를 반환합니다. */
    UFUNCTION(BlueprintCallable, Category = "Lux|Action|Task")
    ELuxTaskLifecycleState GetLifecycleState() const { return LifecycleState; }

    /** 태스크가 활성화된 상태인지 확인합니다. */
    UFUNCTION(BlueprintCallable, Category = "Lux|Action|Task")
    bool IsActive() const { return LifecycleState == ELuxTaskLifecycleState::Executing; }

    /** 태스크가 종료된 상태인지 확인합니다. */
    UFUNCTION(BlueprintCallable, Category = "Lux|Action|Task")
    bool IsEnded() const { return LifecycleState == ELuxTaskLifecycleState::Ended; }

    /** 예측용 인스턴스에서 서버에서 복제된 인스턴스로 소유권을 이전할 때 호출됩니다. */
    void ReHome(ULuxAction* NewOwningAction);

    /** FBaseLuxActionTaskParams 에 따라 초기화합니다. */
    virtual void InitializeFromStruct(const FInstancedStruct& Struct) {}

protected:
    /** Activate()가 호출된 직후 실제 활성화 로직을 처리하기 위해 호출됩니다. */
    virtual void OnActivated();

    /** 태스크가 종료될 때 실제 정리 로직을 처리하기 위해 호출됩니다. 
     * @param bSuccess - 태스크의 성공 여부
     */
    virtual void OnEnded(bool bSuccess);

    /** 태스크의 소유권이 이전되기 직전에 호출됩니다. */
    virtual void OnBeforeReHome();

    /** 태스크의 소유권이 새로운 액션으로 이전된 직후에 호출됩니다.  */
    virtual void OnAfterReHome();

public:
	/** 이 태스크가 속한 액션 시스템 컴포넌트입니다. */
    UPROPERTY()
	FName TaskName;

    /** 이 태스크를 소유하고 있는 액션 인스턴스입니다. */
    UPROPERTY()
    TWeakObjectPtr <ULuxAction> OwningAction;

protected:
    /** 태스크의 종료 여부를 나타내는 스레드 안전 플래그입니다. */
    FThreadSafeBool bIsEnded;

    UPROPERTY()
    ELuxTaskLifecycleState LifecycleState;
};
