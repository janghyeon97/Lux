// Copyright Epic Games, Inc. All Rights Reserved.

#include "HealthTargetFilter.h"
#include "LuxLogChannels.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/ActorComponent.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Attributes/ResourceSet.h"

// 체력 관련 컴포넌트 인터페이스 (프로젝트에 맞게 수정 필요)
// #include "ActionSystem/Attributes/CombatSet.h"
// #include "AbilitySystemComponent.h"

UHealthTargetFilter::UHealthTargetFilter()
{
	FilterMode = EHealthFilterMode::AliveOnly;
	MinHealth = 0.0f;
	MaxHealth = 100.0f;
	MinHealthPercentage = 0.0f;
	MaxHealthPercentage = 1.0f;
	LowHealthThreshold = 0.3f;
	bDebugLog = false;
}

bool UHealthTargetFilter::IsTargetValid(AActor* Target, AActor* SourceActor) const
{
	if (::IsValid(Target) == false)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[HealthTargetFilter] Target is null"));
		}
		return false;
	}

	float CurrentHealth = GetTargetCurrentHealth(Target);
	float MaxHealth_Value = GetTargetMaxHealth(Target);
	float HealthPercentage = GetTargetHealthPercentage(Target);

	bool bIsValid = false;

	switch (FilterMode)
	{
	case EHealthFilterMode::AliveOnly:
		bIsValid = CurrentHealth > 0.0f;
		break;

	case EHealthFilterMode::DeadOnly:
		bIsValid = CurrentHealth <= 0.0f;
		break;

	case EHealthFilterMode::HealthRange:
		bIsValid = (CurrentHealth >= MinHealth) && (CurrentHealth <= MaxHealth);
		break;

	case EHealthFilterMode::HealthPercentageRange:
		bIsValid = (HealthPercentage >= MinHealthPercentage) && (HealthPercentage <= MaxHealthPercentage);
		break;

	case EHealthFilterMode::LowHealth:
		bIsValid = HealthPercentage <= LowHealthThreshold;
		break;

	default:
		bIsValid = true;
		break;
	}

	if (bDebugLog)
	{
		FString ModeString;
		switch (FilterMode)
		{
		case EHealthFilterMode::AliveOnly: ModeString = TEXT("AliveOnly"); break;
		case EHealthFilterMode::DeadOnly: ModeString = TEXT("DeadOnly"); break;
		case EHealthFilterMode::HealthRange: ModeString = TEXT("HealthRange"); break;
		case EHealthFilterMode::HealthPercentageRange: ModeString = TEXT("HealthPercentageRange"); break;
		case EHealthFilterMode::LowHealth: ModeString = TEXT("LowHealth"); break;
		default: ModeString = TEXT("Unknown"); break;
		}

		UE_LOG(LogLux, Log, TEXT("[HealthTargetFilter] %s: Mode=%s, Health=%.1f/%.1f (%.1f%%), Valid=%s"), 
			*Target->GetName(), *ModeString, CurrentHealth, MaxHealth_Value, HealthPercentage * 100.0f,
			bIsValid ? TEXT("Yes") : TEXT("No"));
	}

	return bIsValid;
}

FString UHealthTargetFilter::GetFilterDescription() const
{
	FString Description = TEXT("Health: ");

	switch (FilterMode)
	{
	case EHealthFilterMode::AliveOnly:
		Description += TEXT("Alive only");
		break;

	case EHealthFilterMode::DeadOnly:
		Description += TEXT("Dead only");
		break;

	case EHealthFilterMode::HealthRange:
		Description += FString::Printf(TEXT("%.0f - %.0f HP"), MinHealth, MaxHealth);
		break;

	case EHealthFilterMode::HealthPercentageRange:
		Description += FString::Printf(TEXT("%.0f%% - %.0f%%"), MinHealthPercentage * 100.0f, MaxHealthPercentage * 100.0f);
		break;

	case EHealthFilterMode::LowHealth:
		Description += FString::Printf(TEXT("<= %.0f%%"), LowHealthThreshold * 100.0f);
		break;

	default:
		Description += TEXT("Unknown mode");
		break;
	}

	return Description;
}

float UHealthTargetFilter::GetTargetCurrentHealth(AActor* Target) const
{
	if (::IsValid(Target) == false)
	{
		return 0.0f;
	}

	UActionSystemComponent* ASC = nullptr;

	IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(Target);
	if (ASCInterface)
	{
		ASC = ASCInterface->GetActionSystemComponent();
	}

	if (!ASC)
	{
		ASC = Target->FindComponentByClass<UActionSystemComponent>();
	}

	if (!ASC)
	{
		return 0.0f;
	}

	const UResourceSet* ResourceSet = ASC->GetAttributeSet<UResourceSet>();
	if (!ResourceSet)
	{
		return 0.0f;
	}

	return ResourceSet->GetHealth();
}

float UHealthTargetFilter::GetTargetMaxHealth(AActor* Target) const
{
	if (::IsValid(Target) == false)
	{
		return 0.0f;
	}

	UActionSystemComponent* ASC = nullptr;

	IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(Target);
	if (ASCInterface)
	{
		ASC = ASCInterface->GetActionSystemComponent();
	}

	if (!ASC)
	{
		ASC = Target->FindComponentByClass<UActionSystemComponent>();
	}

	if (!ASC)
	{
		return 0.0f;
	}

	const UResourceSet* ResourceSet = ASC->GetAttributeSet<UResourceSet>();
	if (!ResourceSet)
	{
		return 0.0f;
	}

	return ResourceSet->GetMaxHealth();
}

float UHealthTargetFilter::GetTargetHealthPercentage(AActor* Target) const
{
	float CurrentHealth = GetTargetCurrentHealth(Target);
	float MaxHealth_Value = GetTargetMaxHealth(Target);

	if (MaxHealth_Value <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(CurrentHealth / MaxHealth_Value, 0.0f, 1.0f);
}
