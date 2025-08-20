// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "LuxEffectData.generated.h"

class ULuxEffect;



/**
 * 게임에서 사용되는 기본 이펙트들의 템플릿을 관리하는 데이터 에셋입니다.
 * SetByCaller를 통해 피해 계산에 필요한 초기 변수들을 전달받도록 설계되었습니다.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Lux Effect Data", ShortTooltip = "Data asset containing default effect templates."))
class LUX_API ULuxEffectData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULuxEffectData();

	// 편의 함수들
	UFUNCTION(BlueprintPure, Category = "Effect Access")
	TSubclassOf<ULuxEffect> GetInstantDamageEffect() const;

	UFUNCTION(BlueprintPure, Category = "Effect Access")
	TSubclassOf<ULuxEffect> GetPeriodicDamageEffect() const;

	UFUNCTION(BlueprintPure, Category = "Effect Access")
	TSubclassOf<ULuxEffect> GetInstantHealEffect() const;

	UFUNCTION(BlueprintPure, Category = "Effect Access")
	TSubclassOf<ULuxEffect> GetPeriodicHealEffect() const;

	UFUNCTION(BlueprintPure, Category = "Effect Access")
	TSubclassOf<ULuxEffect> GetCooldownEffect() const;

	UFUNCTION(BlueprintPure, Category = "Effect Access")
	TSubclassOf<ULuxEffect> GetCostEffect() const;

	UFUNCTION(BlueprintPure, Category = "Effect Access")
	TSubclassOf<ULuxEffect> GetDynamicTagEffect() const;


public:
	/*
	 * 즉시 피해를 주기 위한 ULuxEffect의 템플릿입니다.
	 *
	 * SetByCallerMagnitude를 통해 피해 계산에 필요한 초기 변수들(기본 물리/마법 피해량, 물리/마법 계수)을 전달받도록 설계되었습니다.
	 * LuxGameplayTags::Effect_SetByCaller_Damage_Base_Physical - 기본 물리 피해량
	 * LuxGameplayTags::Effect_SetByCaller_Damage_Base_Magical - 기본 마법 피해량
	 * LuxGameplayTags::Effect_SetByCaller_Damage_Scale_Physical - 물리 피해량 계수
	 * LuxGameplayTags::Effect_SetByCaller_Damage_Scale_Magical - 마법 피해량 계수
	 * 
	 * 피해량을 최종 계산하는 ULuxExecutionCalculation_Damage 클래스가 추가되어 있습니다.
	 * Execution은 전달받은 SetByCaller 값들과 시전자 및 대상의 현재 스탯(공격력, 방어력 등)을
	 * 종합하여 최종 피해량을 결정하고, 그 값을 담은 새로운 FAttributeModifier를 Spec에 동적으로
	 * 추가하여 실제 피해를 적용하는 역할을 담당합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Effects", meta = (DisplayName = "Instant Damage Effect (SetByCaller)"))
	TSoftClassPtr<ULuxEffect> InstantDamageEffect;

	// 주기적인 대미지를 가하는 데 사용되는 Lux Effect입니다. SetByCaller로 대미지 양과 지속 시간을 결정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Damage Effects", meta = (DisplayName = "Periodic Damage Effect (SetByCaller)"))
	TSoftClassPtr<ULuxEffect> PeriodicDamageEffect;

	// 즉시 치유를 하는 데 사용되는 Lux Effect입니다. SetByCaller로 치유량을 결정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Healing Effects", meta = (DisplayName = "Instant Heal Effect (SetByCaller)"))
	TSoftClassPtr<ULuxEffect> InstantHealEffect;

	// 주기적인 치유를 하는 데 사용되는 Lux Effect입니다. SetByCaller로 치유량과 지속 시간을 결정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Healing Effects", meta = (DisplayName = "Periodic Heal Effect (SetByCaller)"))
	TSoftClassPtr<ULuxEffect> PeriodicHealEffect;

	/*
	 * Cooldown 효과를 주기 위한 Lux Effect입니다.
	 * LuxGameplayTags::Effect_SetByCaller_Cooldown - 지속 시간.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Utility Effects", meta = (DisplayName = "Generic Cooldown Effect (SetByCaller)"))
	TSoftClassPtr<ULuxEffect> CooldownEffect;

	/*
	 * Cost 효과를 주기 위한 Lux Effect입니다. 기본은 마나를 소모합니다.
	 * LuxGameplayTags::Effect_SetByCaller_Cost - 소모할 자원 양.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Utility Effects", meta = (DisplayName = "Generic Cost Effect (SetByCaller)"))
	TSoftClassPtr<ULuxEffect> CostEffect;

	// 동적 태그 추가/제거에 사용되는 Lux Effect입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Dynamic Tags")
	TSoftClassPtr<ULuxEffect> DynamicTagEffect;
};
