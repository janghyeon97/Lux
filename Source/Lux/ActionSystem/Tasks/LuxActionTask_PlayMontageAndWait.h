// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "Animation/AnimInstance.h"
#include "ActionSystem/Actions/LuxDynamicValue.h"
#include "LuxActionTask_PlayMontageAndWait.generated.h"

class UActionSystemComponent;


/**
 * ULuxActionTask_PlayMontageAndWait Task를 위한 초기화 파라미터입니다.
 */
USTRUCT(BlueprintType)
struct FPlayMontageAndWaitParams : public FBaseLuxActionTaskParams
{
	GENERATED_BODY()

public:
	/** 재생할 몽타주 애셋입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayMontageAndWait")
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** 재생 속도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayMontageAndWait")
	FDynamicFloat Rate;

	/** 몽타주를 시작할 섹션의 이름입니다. (비워두면 처음부터 시작) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayMontageAndWait")
	FName StartSection;

	/** 액션이 종료될 때 몽타주 재생을 강제로 중지할지 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayMontageAndWait")
	bool bStopWhenAbilityEnds = true;
};


/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_PlayMontageAndWait : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
	/**
	 * 몽타주 재생 및 대기 Task를 생성하고 활성화 준비를 합니다.
	 * @param InOwningAction 이 태스크를 소유하는 액션입니다.
	 * @param MontageToPlay 재생할 몽타주 애셋입니다.
	 * @param Rate 재생 속도입니다.
	 * @param StartSection 몽타주를 시작할 섹션의 이름입니다. (지정하지 않으면 처음부터 시작)
	 * @param bStopWhenAbilityEnds 액션이 종료될 때 몽타주 재생을 강제로 중지할지 여부입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Play Montage And Wait"))
	static ULuxActionTask_PlayMontageAndWait* PlayMontageAndWait(
		ULuxAction* InOwningAction,
		UAnimMontage* MontageToPlay,
		float Rate = 1.0f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true
	);

protected:
	// ~ULuxActionTask interface
	virtual void InitializeFromStruct(const FInstancedStruct& Struct) override;
	virtual void OnActivated() override;
	virtual void OnEnded(bool bSuccess) override;
	virtual void OnBeforeReHome() override;
	virtual void OnAfterReHome() override;
	//~End of ULuxActionTask interface

	// AnimInstance의 델리게이트에 바인딩될 콜백 함수들입니다.
	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

private:
	/** 델리게이트를 모두 해제하는 함수입니다. */
	void UnbindDelegates();

	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY()
	float Rate;

	UPROPERTY()
	FName StartSection;

	UPROPERTY()
	bool bStopWhenAbilityEnds;

	/** 몽타주를 건너뛰었는지 여부입니다. */
	bool bWasSkipped = false;

	/** 현재 블렌드 아웃 중인지 여부입니다. */
	bool bIsBlendingOut = false;

	/** 몽타주가 정상적으로 종료되었는지 여부입니다. */
	bool bWasCompleted = false;
};
