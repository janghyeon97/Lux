// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "DefenseSet.generated.h"

/**
 * 
 */
UCLASS()
class LUX_API UDefenseSet : public ULuxAttributeSet
{
	GENERATED_BODY()
	
public:
	UDefenseSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ATTRIBUTE_ACCESSORS(UDefenseSet, Armor)
	ATTRIBUTE_ACCESSORS(UDefenseSet, MagicResistance)
	ATTRIBUTE_ACCESSORS(UDefenseSet, Tenacity)

	// --- Delegates for UI update ---
	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnArmorChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnMagicResistanceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnTenacityChanged;

protected:
	virtual void PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue) override;

	UFUNCTION()
	virtual void OnRep_Armor(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MagicResistance(const FLuxAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Tenacity(const FLuxAttributeData& OldValue);

protected:
	/** 방어력입니다. 받는 물리 피해량을 감소시킵니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Attributes|Defense")
	FLuxAttributeData Armor;

	/** 마법 저항력입니다. 받는 마법 피해량을 감소시킵니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicResistance, Category = "Attributes|Defense")
	FLuxAttributeData MagicResistance;

	/** 강인함입니다. 받는 군중 제어(CC) 효과의 지속시간을 감소시킵니다. (%) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Tenacity, Category = "Attributes|Defense")
	FLuxAttributeData Tenacity;
};
