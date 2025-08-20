// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Executions/LuxExecution_AttackSpeedCooldown.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Attributes/CombatSet.h"
#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

void ULuxExecution_AttackSpeedCooldown::Execute_Implementation(FLuxEffectSpec& Spec) const
{
	const FLuxEffectContextHandle& ContextHandle = Spec.ContextHandle;
	if (!ContextHandle.IsValid()) return;

	const FLuxActionSpecHandle ActionSpecHandle = ContextHandle.GetSourceAction();
	if (!ActionSpecHandle.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 액션 스펙 핸들이 유효하지 않습니다."));
		return;
	}

	UActionSystemComponent* SourceASC = ContextHandle.GetSourceASC();
	if (!SourceASC)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 소스 액션 시스템 컴포넌트가 유효하지 않습니다."));
		return;
	}

	const FLuxActionSpec* ActionSpec = SourceASC->FindActionSpecFromHandle(ActionSpecHandle);
	if (!ActionSpec)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 핸들로부터 액션 스펙을 찾을 수 없습니다."));
		return;
	}

	const UCombatSet* CombatSet = SourceASC->GetAttributeSet<UCombatSet>();
	if (!CombatSet)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("AttackSpeed Cooldown Execution failed: Source CombatSet is invalid."));
		return;
	}

	const float AttackSpeed = CombatSet->GetAttackSpeed();
	if (AttackSpeed <= 0.f)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("AttackSpeed Cooldown Execution failed: AttackSpeed is zero or negative."));
		return;
	}

	// 공격 속도를 기반으로 쿨다운(공격 주기)을 계산합니다. (1 / 초당 공격 횟수)
	const float CooldownDuration = 1.0f / AttackSpeed;
	Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Duration, CooldownDuration);

	const FGameplayTag CooldownTag = ActionSpec->GetCooldownTag();
	if (CooldownTag.IsValid())
	{
		Spec.DynamicGrantedTags.AddTag(CooldownTag);
	}
}
