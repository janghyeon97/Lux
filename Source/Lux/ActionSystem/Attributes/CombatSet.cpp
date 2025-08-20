// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Attributes/CombatSet.h"
#include "Net/UnrealNetwork.h"


UCombatSet::UCombatSet()
{
	InitAttackDamage(10.f);
	InitAbilityPower(0.f);
	InitAttackSpeed(0.625f);
	InitActionHaste(0.f);
	InitCritChance(0.f);
	InitCritDamage(175.f);
}

void UCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UCombatSet, AttackDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatSet, AbilityPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatSet, ActionHaste, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatSet, CritDamage, COND_None, REPNOTIFY_Always);
}

void UCombatSet::PreAttributeBaseChange(const FLuxAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

}

void UCombatSet::PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetActionHasteAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetCritChanceAttribute())
	{
		// 치명타 확률은 0%와 100% 사이의 값을 가집니다.
		NewValue = FMath::Clamp(NewValue, 0.f, 100.f);
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		// 공격 속도는 최대 한계치가 있습니다. (예: 초당 2.5회)
		NewValue = FMath::Clamp(NewValue, 0.1f, 2.5f);
	}
}


void UCombatSet::PostAttributeChange(const FLuxAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 특정 능력치가 변경되었을 때 추가적인 로직이 필요하다면 여기에 작성합니다.
	// 예를 들어, 공격 속도가 변경되면 캐릭터의 애니메이션 속도도 함께 조절할 수 있습니다.
}

void UCombatSet::OnRep_AttackDamage(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UCombatSet, AttackDamage, OldValue);
}

void UCombatSet::OnRep_AbilityPower(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UCombatSet, AbilityPower, OldValue);
}

void UCombatSet::OnRep_AttackSpeed(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UCombatSet, AttackSpeed, OldValue);
}

void UCombatSet::OnRep_ActionHaste(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UCombatSet, ActionHaste, OldValue);
}

void UCombatSet::OnRep_CritChance(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UCombatSet, CritChance, OldValue);
}

void UCombatSet::OnRep_CritDamage(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UCombatSet, CritDamage, OldValue);
}