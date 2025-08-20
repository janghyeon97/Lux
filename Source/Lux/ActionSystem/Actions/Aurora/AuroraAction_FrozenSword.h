// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "AuroraAction_FrozenSword.generated.h"

class UAnimMontage;
class ULuxEffect;

/**
 * 
 */
UCLASS()
class LUX_API UAuroraAction_FrozenSword : public ULuxAction
{
	GENERATED_BODY()
	

public:
	UAuroraAction_FrozenSword();

protected:
	/** 각 페이즈 진입 시 실행되는 로직입니다. */
	virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC) override;
	virtual void OnActionEnd(bool bIsCancelled) override;
	//~ End ULuxAction Overrides

private:
	/** Begin 페이즈 진입 시 실행되는 로직입니다. */
	void PhaseBegin(UActionSystemComponent& SourceASC);

	/** 타격 판정을 처리하는 페이즈입니다. (서버 전용) */
	void PhaseProcessHit(UActionSystemComponent& SourceASC);

	/** Recovery 페이즈 진입 시 실행되는 로직입니다. */
	void PhaseRecovery(UActionSystemComponent& SourceASC);

	/** Interrupt 페이즈 진입 시 실행되는 로직입니다. */
	void PhaseInterrupt(UActionSystemComponent& SourceASC);

	/** End 페이즈 진입 시 실행되는 로직입니다. */
	void PhaseEnd(UActionSystemComponent& SourceASC);

	/** 공격 속도에 따라 몽타주를 재생하는 함수입니다. */
	void PlayMontageWithAttackSpeed(UActionSystemComponent& SourceASC, int32 ComboCount);

	/** 클라이언트에서 타격 판정을 예측하고 서버에 검증을 요청합니다. */
	UFUNCTION()
	void Client_PredictHitCheck();

	/** 서버에서 클라이언트의 HitCheck 요청 이벤트를 수신하여 처리합니다. */
	UFUNCTION()
	void Server_OnHitPredicted(const FGameplayTag& EventTag, const FContextPayload& Payload);

protected:
	/** 기본 공격 애니메이션 몽타주입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** 콤보 카운트를 추적하기 위한 게임플레이 태그입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Tag")
	FGameplayTag ComboCountTag;

	UPROPERTY(EditDefaultsOnly, Category = "Tag")
	float ComboResetTime = 5.0f;
};
