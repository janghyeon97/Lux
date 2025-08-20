// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ResourceSet.generated.h"




/**
 * 
 */
UCLASS()
class LUX_API UResourceSet : public ULuxAttributeSet
{
	GENERATED_BODY()
	
public:
	UResourceSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Attribute Accessors ---
	ATTRIBUTE_ACCESSORS(UResourceSet, Health)
	ATTRIBUTE_ACCESSORS(UResourceSet, MaxHealth)
	ATTRIBUTE_ACCESSORS(UResourceSet, Mana)
	ATTRIBUTE_ACCESSORS(UResourceSet, MaxMana)
	ATTRIBUTE_ACCESSORS(UResourceSet, HealthRegen)
	ATTRIBUTE_ACCESSORS(UResourceSet, ManaRegen)

	// --- Delegates for UI update ---
	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnHealthRegenChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnMaxManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnManaRegenChanged;
 
protected:
	/** BaseValue 값이 변경되기 직전에 호출됩니다. */
	virtual void PreAttributeBaseChange(const FLuxAttribute& Attribute, float& NewValue) override;

	/** CurrentValue 값이 변경되기 직전에 호출됩니다. */
	virtual void PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue) override;

	/** CurrentValue 값이 변경된 직후에 호출됩니다. */
	virtual void PostAttributeChange(const FLuxAttribute& Attribute, float OldValue, float NewValue) override;

	/** 이펙트가 속성에 적용된 직후에 호출됩니다. */
	virtual void PostLuxEffectExecute(const FLuxModCallbackData& Data) override;

	UFUNCTION()
	virtual void OnRep_Health(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_HealthRegen(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Mana(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxMana(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_ManaRegen(const FLuxAttributeData& OldValue);

protected:
	/** 현재 체력입니다. 0이 되면 캐릭터는 사망합니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Health")
	FLuxAttributeData Health;

	/** 최대 체력입니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Health")
	FLuxAttributeData MaxHealth;

	/** 5초당 체력 재생량입니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegen, Category = "Attributes|Health")
	FLuxAttributeData HealthRegen;

	/** 현재 마나(스킬 자원)입니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Attributes|Mana")
	FLuxAttributeData Mana;

	/** 최대 마나입니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Attributes|Mana")
	FLuxAttributeData MaxMana;

	/** 5초당 마나 재생량입니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ManaRegen, Category = "Attributes|Mana")
	FLuxAttributeData ManaRegen;

protected:
	void ClampAttribute(const FLuxAttribute& Attribute, float& NewValue) const;
};
