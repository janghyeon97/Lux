// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "LuxActionTask_LandingControl.generated.h"

class UAnimMontage;

/**
 * 캐릭터의 착지를 제어하는 태스크입니다.
 * 지정된 시간 동안 캐릭터의 Z축 속도를 조절하여 자연스럽게 착지시킵니다.
 */
UCLASS()
class LUX_API ULuxActionTask_LandingControl : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
	/**
	 * 착지 제어 태스크를 생성하고 활성화합니다.
	 * @param InOwningAction 이 태스크를 소유하는 액션입니다.
	 * @param InLandingVelocity 착지 시 적용할 Z축 속도입니다.
	 * @param InLandingDuration 착지 제어 지속 시간입니다.
	 * @param InInterpSpeed 속도 보간 속도입니다 (자연 착지 시 사용).
	 * @param InUseNaturalLanding 자연스러운 착지 사용 여부입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Landing Control"))
	static ULuxActionTask_LandingControl* LandingControl(
		ULuxAction* InOwningAction,
		float InLandingVelocity,
		float InLandingDuration,
		float InInterpSpeed = 5.0f,
		bool InUseNaturalLanding = true
	);

protected:
	// ~ULuxActionTask interface
	virtual void OnActivated() override;
	virtual void OnEnded(bool bSuccess) override;
	virtual void OnBeforeReHome() override;
	//~End of ULuxActionTask interface

	/** 착지 제어를 업데이트하는 함수입니다. */
	void UpdateLandingControl();

	/** 자연스러운 착지를 위한 속도를 계산하는 함수입니다. */
	float CalculateNaturalLandingVelocity(float CurrentVelocityZ);

	/** 기본 착지 속도를 계산하는 함수입니다. */
	float CalculateBasicLandingVelocity() const;

private:
	/** 델리게이트를 해제하는 함수입니다. */
	void UnbindDelegates();

	UPROPERTY()
	float LandingVelocity;

	UPROPERTY()
	float LandingDuration;

	UPROPERTY()
	float InterpSpeed;

	UPROPERTY()
	bool bUseNaturalLanding;

	UPROPERTY()
	float ElapsedTime;

	UPROPERTY()
	float StartHeight;

	UPROPERTY()
	float CurrentHeight;

	UPROPERTY()
	FTimerHandle UpdateTimerHandle;


};
