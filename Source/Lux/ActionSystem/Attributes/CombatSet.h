// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "CombatSet.generated.h"


/**
 * 
 */
UCLASS()
class LUX_API UCombatSet : public ULuxAttributeSet
{
	GENERATED_BODY()
	
public:
	UCombatSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ATTRIBUTE_ACCESSORS(UCombatSet, AttackDamage)
	ATTRIBUTE_ACCESSORS(UCombatSet, AbilityPower)
	ATTRIBUTE_ACCESSORS(UCombatSet, AttackSpeed)
	ATTRIBUTE_ACCESSORS(UCombatSet, ActionHaste)
	ATTRIBUTE_ACCESSORS(UCombatSet, CritChance)
	ATTRIBUTE_ACCESSORS(UCombatSet, CritDamage)

	// --- Delegates for UI update ---
	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnAttackDamageChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnAbilityPowerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnAttackSpeedChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnActionHasteChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnCritChanceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnCritDamageChanged;

protected:
	/** BaseValue 값이 변경되기 직전에 호출됩니다. */
	virtual void PreAttributeBaseChange(const FLuxAttribute& Attribute, float& NewValue) override;

	/** CurrentValue 값이 변경되기 직전에 호출됩니다. */
	virtual void PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue) override;

	/** CurrentValue 값이 변경된 직후에 호출됩니다. */
	virtual void PostAttributeChange(const FLuxAttribute& Attribute, float OldValue, float NewValue) override;

	UFUNCTION()
	virtual void OnRep_AttackDamage(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_AbilityPower(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_AttackSpeed(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_ActionHaste(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_CritChance(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_CritDamage(const FLuxAttributeData& OldValue);

protected:
	/** 물리 공격력입니다. 기본 공격 및 물리 스킬 피해량에 영향을 줍니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackDamage, Category = "Attributes|Combat")
	FLuxAttributeData AttackDamage;

	/** 주문력입니다. 마법 스킬 피해량에 영향을 줍니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityPower, Category = "Attributes|Combat")
	FLuxAttributeData AbilityPower;

	/** 공격 속도입니다. (초당 공격 횟수) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Attributes|Combat")
	FLuxAttributeData AttackSpeed;

	/** 액션 가속입니다. 액션의 재사용 대기시간을 줄여줍니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActionHaste, Category = "Attributes|Combat")
	FLuxAttributeData ActionHaste;

	/** 치명타 확률입니다. (%) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance, Category = "Attributes|Combat")
	FLuxAttributeData CritChance;

	/** 치명타 피해량 증가율입니다. (%) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritDamage, Category = "Attributes|Combat", meta = (ClampMin = 0.0, ClampMax = 1000.0))
	FLuxAttributeData CritDamage;
};
