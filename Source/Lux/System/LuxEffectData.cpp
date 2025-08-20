// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuxEffectData.h"
#include "ActionSystem/Effects/LuxEffect.h"
#include "System/LuxAssetManager.h"
#include "LuxLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxEffectData)

ULuxEffectData::ULuxEffectData()
{
}

TSubclassOf<ULuxEffect> ULuxEffectData::GetInstantDamageEffect() const
{
	return ULuxAssetManager::Get().GetSubclass(InstantDamageEffect);
}

TSubclassOf<ULuxEffect> ULuxEffectData::GetPeriodicDamageEffect() const
{
	return ULuxAssetManager::Get().GetSubclass(PeriodicDamageEffect);
}

TSubclassOf<ULuxEffect> ULuxEffectData::GetInstantHealEffect() const
{
	return ULuxAssetManager::Get().GetSubclass(InstantHealEffect);
}

TSubclassOf<ULuxEffect> ULuxEffectData::GetPeriodicHealEffect() const
{
	return ULuxAssetManager::Get().GetSubclass(PeriodicHealEffect);
}

TSubclassOf<ULuxEffect> ULuxEffectData::GetCooldownEffect() const
{
	return ULuxAssetManager::Get().GetSubclass(CooldownEffect);
}

TSubclassOf<ULuxEffect> ULuxEffectData::GetCostEffect() const
{
	return ULuxAssetManager::Get().GetSubclass(CostEffect);
}

TSubclassOf<ULuxEffect> ULuxEffectData::GetDynamicTagEffect() const
{
	return ULuxAssetManager::Get().GetSubclass(DynamicTagEffect);
}
