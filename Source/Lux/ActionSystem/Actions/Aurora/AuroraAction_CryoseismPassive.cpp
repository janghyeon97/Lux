// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Actions/Aurora/AuroraAction_CryoseismPassive.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Cooldown/LuxCooldownTracker.h"

#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

UAuroraAction_CryoseismPassive::UAuroraAction_CryoseismPassive()
{
	// 패시브 액션 설정: 액터당 하나의 인스턴스가 메모리에 상주
	InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerActor;

	// 부여될 때 실행되는 액션으로 설정
	ActivationPolicy = ELuxActionActivationPolicy::OnGrant;

	// 액션 식별자 태그 설정
	ActionIdentifierTag = LuxActionTags::Action_Aurora_CryoseismPassive;

	// Cryoseism 액션을 대상으로 설정
	TargetActionTag = LuxActionTags::Action_Aurora_Cryoseism;

	// 기본 쿨다운 감소량 설정
	CooldownReduction = 1.0f;
}

void UAuroraAction_CryoseismPassive::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
	Super::OnPhaseEnter(PhaseTag, SourceASC);

	if (PhaseTag == LuxPhaseTags::Phase_Action_Begin)		PhaseBegin(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_End)		EndAction();
}

void UAuroraAction_CryoseismPassive::OnActionEnd(bool bIsCancelled)
{
	Super::OnActionEnd(bIsCancelled);

	UActionSystemComponent* SourceASC = GetActionSystemComponent();
	if (SourceASC)
	{
		SourceASC->UnsubscribeFromGameplayEvent(LuxGameplayTags::Event_Character_DealtDamage, OnBasicAttackHitDelegate);
	}
}

void UAuroraAction_CryoseismPassive::PhaseBegin(UActionSystemComponent& SourceASC)
{
	TargetSpec = SourceASC.FindActionSpecByIdentifierTag(TargetActionTag);
	if (!TargetSpec)
	{
		UE_LOG(LogLuxCooldown, Warning, TEXT("[%s] ExecuteCooldownReduction: 대상 액션 '%s'을(를) 찾을 수 없습니다."),
			*GetLogPrefix(), *TargetActionTag.ToString());
		return;
	}

	CooldownTracker = SourceASC.GetCooldownTracker();

	TargetInputTag = TargetSpec->InputTag;
	TargetCooldownTag = TargetSpec->GetCooldownTag();

	OnBasicAttackHitDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UAuroraAction_CryoseismPassive, ExecuteCooldownReduction));
	SourceASC.SubscribeToGameplayEvent(LuxGameplayTags::Event_Character_DealtDamage, OnBasicAttackHitDelegate);
}

void UAuroraAction_CryoseismPassive::ExecuteCooldownReduction(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	const FPayload_DamageEvent* DamagePayload = Payload.GetData<FPayload_DamageEvent>(LuxPayloadKeys::DamageEvent);
	if (DamagePayload->SourceActionTypeTag != LuxGameplayTags::Action_Type_Attack)
	{
		// 공격 액션이 아닌 경우 쿨다운 감소를 실행하지 않습니다.
		return;
	}

	if (!::IsValid(CooldownTracker))
	{
		UE_LOG(LogLuxCooldown, Error, TEXT("[%s] ExecuteCooldownReduction: CooldownTracker를 찾을 수 없습니다."),
			*GetLogPrefix());
		return;
	}

	// 대상 액션(Cryoseism)의 Spec을 찾습니다.
	if (!TargetSpec)
	{
		UE_LOG(LogLuxCooldown, Warning, TEXT("[%s] ExecuteCooldownReduction: 대상 액션 '%s'을(를) 찾을 수 없습니다."),
			*GetLogPrefix(), *TargetActionTag.ToString());
		return;
	}

	if(TargetSpec->Level < 1)
	{
		UE_LOG(LogLuxCooldown, Warning, TEXT("[%s] ExecuteCooldownReduction: 대상 액션 '%s'의 레벨이 0입니다."),
			*GetLogPrefix(), *TargetActionTag.ToString());
		return;
	}

	if (TargetInputTag != TargetSpec->InputTag)
	{
		TargetCooldownTag = TargetSpec->GetCooldownTag();
	}

	// 쿨다운 트래커를 통해 쿨다운을 감소시킵니다.
	// 현재 쿨다운이 활성화되어 있는지 확인합니다.
	float CurrentTimeRemaining = CooldownTracker->GetTimeRemaining(TargetCooldownTag);
	if (CurrentTimeRemaining > 0.0f)
	{
		CooldownTracker->ReduceCooldown(TargetCooldownTag, CooldownReduction);

		UE_LOG(LogLuxCooldown, Log, TEXT("[%s] ExecuteCooldownReduction: '%s' 쿨다운을 %.1f초 감소시켰습니다. (남은 시간: %.2f초 -> %.2f초)"),
			*GetLogPrefix(), *TargetCooldownTag.ToString(), CooldownReduction,
			CurrentTimeRemaining, CooldownTracker->GetTimeRemaining(TargetCooldownTag));
	}
	else
	{
		UE_LOG(LogLuxCooldown, Log, TEXT("[%s] ExecuteCooldownReduction: '%s'이(가) 현재 쿨다운 상태가 아닙니다."),
			*GetLogPrefix(), *TargetCooldownTag.ToString());
	}
}
