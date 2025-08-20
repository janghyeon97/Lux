// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxActionTypes.h"
#include "AuroraAction_CryoseismPassive.generated.h"

class ULuxCooldownTracker;

/**
 * Aurora의 패시브 액션 - 기본 공격 적중 시 Cryoseism(궁극기) 쿨다운을 감소시킵니다.
 * 
 * 특징:
 * - InstancedPerActor 정책으로 액터당 하나의 인스턴스가 메모리에 상주
 * - EventTriggerTags를 통해 기본 공격 적중 이벤트에 자동으로 반응
 * - 즉시 실행 후 종료되는 패시브 효과
 */
UCLASS()
class LUX_API UAuroraAction_CryoseismPassive : public ULuxAction
{
	GENERATED_BODY()
	
public:
	UAuroraAction_CryoseismPassive();

protected:
	//~ ULuxAction Overrides
	virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC) override;
	virtual void OnActionEnd(bool bIsCancelled) override;
	//~ End ULuxAction Overrides

	void PhaseBegin(UActionSystemComponent& SourceASC);

private:
	/** 기본 공격 적중 시 Cryoseism 쿨다운 감소 로직을 실행합니다. */
	UFUNCTION()
	void ExecuteCooldownReduction(const FGameplayTag& EventTag, const FContextPayload& Payload);

protected:
	UPROPERTY()
	TObjectPtr<ULuxCooldownTracker> CooldownTracker = nullptr;

	/** Cryoseism 쿨다운 감소량 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive", meta = (ClampMin = "0.0"))
	float CooldownReduction = 1.0f;

	/** 쿨다운 감소를 적용할 대상 액션의 식별자 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive")
	FGameplayTag TargetActionTag = FGameplayTag::EmptyTag;

	UPROPERTY(Transient)
	FGameplayTag TargetInputTag = FGameplayTag::EmptyTag;

	UPROPERTY(Transient)
	FGameplayTag TargetCooldownTag = FGameplayTag::EmptyTag;

	FLuxActionSpec* TargetSpec = nullptr;

	FScriptDelegate OnBasicAttackHitDelegate;
};
