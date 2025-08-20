// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Attributes/DefenseSet.h"
#include "Net/UnrealNetwork.h"

UDefenseSet::UDefenseSet()
{
	InitArmor(20.f);
	InitMagicResistance(20.f);
	InitTenacity(0.f);
}

void UDefenseSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDefenseSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDefenseSet, MagicResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDefenseSet, Tenacity, COND_None, REPNOTIFY_Always);
}

void UDefenseSet::PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetTenacityAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 70.f); // 예시: 최대 70%
	}
}

void UDefenseSet::OnRep_Armor(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UDefenseSet, Armor, OldValue);
}

void UDefenseSet::OnRep_MagicResistance(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UDefenseSet, MagicResistance, OldValue);
}

void UDefenseSet::OnRep_Tenacity(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UDefenseSet, Tenacity, OldValue);
}
