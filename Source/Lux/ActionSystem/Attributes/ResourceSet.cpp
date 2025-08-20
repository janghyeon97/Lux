// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Attributes/ResourceSet.h"
#include "System/LuxCombatManager.h"
#include "Net/UnrealNetwork.h"
#include "LuxLogChannels.h"

UResourceSet::UResourceSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(100.f);
	InitMaxMana(100.f);
}

void UResourceSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 각 속성이 서버에서 변경될 때마다 클라이언트로 전송되도록 설정합니다.
	DOREPLIFETIME_CONDITION_NOTIFY(UResourceSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UResourceSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UResourceSet, HealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UResourceSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UResourceSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UResourceSet, ManaRegen, COND_None, REPNOTIFY_Always);
}

void UResourceSet::PreAttributeBaseChange(const FLuxAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UResourceSet::PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UResourceSet::PostAttributeChange(const FLuxAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] Attribute 변경: %s (Old: %.1f, New: %.1f)"),
		*GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__),
		*Attribute.GetName().ToString(), OldValue, NewValue);

	// 최대 체력이 줄어들 때 현재 체력이 그보다 크면 현재 체력을 최대 체력으로 조정합니다.
	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		UActionSystemComponent* ASC = GetOwningActionSystemComponent();
		check(ASC);
		ASC->ApplyModToAttribute(GetHealthAttribute(), EModifierOperation::Override, NewValue);
	}
	else if (Attribute == GetMaxManaAttribute() && GetMana() > NewValue)
	{
		UActionSystemComponent* ASC = GetOwningActionSystemComponent();
		check(ASC);
		ASC->ApplyModToAttribute(GetManaAttribute(), EModifierOperation::Override, NewValue);
	}
}

void UResourceSet::PostLuxEffectExecute(const FLuxModCallbackData& Data)
{
	Super::PostLuxEffectExecute(Data);
	
	// 데미지 로그는 이제 ULuxExecution_Damage에서 직접 처리하므로 여기서는 제거하였습니다.
	// Health 변화에 대한 추가적인 처리만 필요시 여기에 추가
}

void UResourceSet::OnRep_Health(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UResourceSet, Health, OldValue);
}

void UResourceSet::OnRep_MaxHealth(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UResourceSet, MaxHealth, OldValue);
}

void UResourceSet::OnRep_HealthRegen(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UResourceSet, HealthRegen, OldValue);
}

void UResourceSet::OnRep_Mana(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UResourceSet, Mana, OldValue);
}

void UResourceSet::OnRep_MaxMana(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UResourceSet, MaxMana, OldValue);
}

void UResourceSet::OnRep_ManaRegen(const FLuxAttributeData& OldValue)
{
	LUXATTRIBUTE_REPNOTIFY(UResourceSet, ManaRegen, OldValue);
}

void UResourceSet::ClampAttribute(const FLuxAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}