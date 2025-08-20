// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Executions/LuxExecution_Damage.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"

#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxActionLevelData.h" 
#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "ActionSystem/Effects/LuxEffect.h"

#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "ActionSystem/Attributes/ResourceSet.h"
#include "ActionSystem/Attributes/CombatSet.h"
#include "ActionSystem/Attributes/DefenseSet.h"

#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"
#include "System/LuxCombatManager.h"


void ULuxExecution_Damage::Execute_Implementation(FLuxEffectSpec& Spec) const
{
	// 컨텍스트와 액터 정보 가져오기
	const FLuxEffectContextHandle& ContextHandle = Spec.ContextHandle;
	if (!ContextHandle.IsValid()) return;

	UActionSystemComponent* SourceASC = ContextHandle.GetSourceASC();
	UActionSystemComponent* TargetASC = ContextHandle.GetTargetASC();
	if (!SourceASC || !TargetASC) return;

	UE_LOG(LogLuxActionSystem, Log, TEXT(">> [DamageExecution] '%s'가 '%s'에게 데미지 계산을 시작합니다."), *GetNameSafe(SourceASC->GetOwner()), *GetNameSafe(TargetASC->GetOwner()));

	/* ==== 입력 데이터 추출 ==== */
	const float BasePhysicalDamage = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Base, false, 0.f);
	const float BaseMagicalDamage = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Base, false, 0.f);
	const float PhysicalDamageScale = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Scale, false, 0.f);
	const float MagicalDamageScale = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Scale, false, 0.f);

	/* ==== 시전자와 대상의 능력치 가져오기 ==== */
	const UCombatSet* SourceCombatSet = SourceASC->GetAttributeSet<UCombatSet>();
	const UDefenseSet* TargetDefenseSet = TargetASC->GetAttributeSet<UDefenseSet>();

	const float SourceAttackDamage = SourceCombatSet ? SourceCombatSet->GetAttackDamage() : 0.f;
	const float SourceAbilityPower = SourceCombatSet ? SourceCombatSet->GetAbilityPower() : 0.f;

	const float TargetArmor = TargetDefenseSet ? TargetDefenseSet->GetArmor() : 0.f;
	const float TargetMagicResistance = TargetDefenseSet ? TargetDefenseSet->GetMagicResistance() : 0.f;

	/* ==== 시전자/대상 스탯 정보를 Spec에 저장 ==== */
	Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Damage_Source_AttackDamage, SourceAttackDamage);
	Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Damage_Source_AbilityPower, SourceAbilityPower);
	Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Damage_Target_Armor, TargetArmor);
	Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Damage_Target_MagicResistance, TargetMagicResistance);

	UE_LOG(LogLuxActionSystem, Log, TEXT("  - 기본(물리/마법): %.1f / %.1f | 계수(물리/마법): %.2f / %.2f"), BasePhysicalDamage, BaseMagicalDamage, PhysicalDamageScale, MagicalDamageScale);
	UE_LOG(LogLuxActionSystem, Log, TEXT("  - 시전자(공격력/주문력): %.1f / %.1f | 대상(방어력/마법저항): %.1f / %.1f"), SourceAttackDamage, SourceAbilityPower, TargetArmor, TargetMagicResistance);

	/* ==== 데미지 계산 ==== */
	float TotalDamage = 0.f;
	float PhysicalDamage = 0.f;
	float MagicalDamage = 0.f;
	float RawPhysicalDamage = 0.f;
	float RawMagicalDamage = 0.f;
	float PhysicalMitigation = 0.f;
	float MagicalMitigation = 0.f;

	/* ==== 물리 피해 계산 ==== */
	if (BasePhysicalDamage > 0.f || PhysicalDamageScale > 0.f)
	{
		// 원시 피해량 계산
		RawPhysicalDamage = BasePhysicalDamage + (SourceAttackDamage * PhysicalDamageScale);
		
		// 방어력 적용
		PhysicalMitigation = 100.f / FMath::Max(100.f, 100.f + TargetArmor);
		PhysicalDamage = RawPhysicalDamage * PhysicalMitigation;

		UE_LOG(LogLuxActionSystem, Log, TEXT("  -> 물리 피해: Raw(%.1f) -> Mitigated(%.1f, %.1f%% 감소)"), RawPhysicalDamage, PhysicalDamage, (1.f - PhysicalMitigation) * 100.f);
		TotalDamage += PhysicalDamage;
	}

	/* ==== 마법 피해 계산 ==== */
	if (BaseMagicalDamage > 0.f || MagicalDamageScale > 0.f)
	{
		// 원시 피해량 계산
		RawMagicalDamage = BaseMagicalDamage + (SourceAbilityPower * MagicalDamageScale);
		
		// 마법저항 적용
		MagicalMitigation = 100.f / FMath::Max(100.f, 100.f + TargetMagicResistance);
		MagicalDamage = RawMagicalDamage * MagicalMitigation;

		UE_LOG(LogLuxActionSystem, Log, TEXT("  -> 마법 피해: Raw(%.1f) -> Mitigated(%.1f, %.1f%% 감소)"), RawMagicalDamage, MagicalDamage, (1.f - MagicalMitigation) * 100.f);
		TotalDamage += MagicalDamage;
	}

	/* ==== 치명타 계산 ==== */
	bool bIsCritical = false;
	float CriticalMultiplier = 1.0f;
	
	if (SourceCombatSet)
	{
		const float CritChance = SourceCombatSet->GetCritChance();
		const float CritDamage = SourceCombatSet->GetCritDamage();
		
		// 치명타 확률 체크
		if (FMath::FRand() < CritChance)
		{
			bIsCritical = true;
			CriticalMultiplier = CritDamage;
			TotalDamage *= CriticalMultiplier;
			
			UE_LOG(LogLuxActionSystem, Log, TEXT("  -> 치명타 발생! 배율: %.2f, 최종 피해: %.1f"), CriticalMultiplier, TotalDamage);
		}
	}

	/* ==== 최종 결과를 Spec에 저장 ==== */
	if (TotalDamage > 0.f)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("<< [DamageExecution] 최종 합산 피해량: %.1f. Spec에 기록합니다."), TotalDamage);

		/* ==== 최종 적용될 데미지 정보 저장 ==== */
		Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magnitude, -TotalDamage);
		
		/* ==== 물리 피해 정보 저장 ==== */
		if (PhysicalDamage > 0.f)
		{
			Spec.DynamicGrantedTags.AddTag(LuxGameplayTags::Effect_Type_Physical);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Base, BasePhysicalDamage);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Scale, PhysicalDamageScale);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Raw, RawPhysicalDamage);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Final, PhysicalDamage);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Damage_Formula_Physical_Mitigation, PhysicalMitigation);
		}
		
		/* ==== 마법 피해 정보 저장 ==== */
		if (MagicalDamage > 0.f)
		{
			Spec.DynamicGrantedTags.AddTag(LuxGameplayTags::Effect_Type_Magical);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Base, BaseMagicalDamage);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Scale, MagicalDamageScale);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Raw, RawMagicalDamage);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Final, MagicalDamage);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Damage_Formula_Magical_Mitigation, MagicalMitigation);
		}

		/* ==== 치명타 정보 저장 ==== */
		if (bIsCritical)
		{
			Spec.DynamicGrantedTags.AddTag(LuxGameplayTags::Effect_Type_Critical);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Critical_IsCritical, 1.0f);
			Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Critical_Multiplier, CriticalMultiplier);
		}
	}
}
