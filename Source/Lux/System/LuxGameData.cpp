// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuxGameData.h"
#include "System/LuxAssetManager.h"
#include "System/LuxEffectData.h"
#include "System/LuxCueData.h"
#include "System/LuxCrowdControlData.h"
#include "Targeting/LuxTargetingData.h"
#include "LuxLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxGameData)

ULuxGameData::ULuxGameData()
{
}

const ULuxGameData& ULuxGameData::Get()
{
	return ULuxAssetManager::Get().GetGameData();
}

ULuxEffectData* ULuxGameData::GetEffectData() const
{
	return ULuxAssetManager::Get().GetAsset(EffectData);
}

ULuxCueData* ULuxGameData::GetCueData() const
{
	return ULuxAssetManager::Get().GetAsset(CueData);
}

ULuxCrowdControlData* ULuxGameData::GetCrowdControlData() const
{
	return ULuxAssetManager::Get().GetAsset(CrowdControlData);
}

ULuxTargetingData* ULuxGameData::GetTargetingData() const
{
	return ULuxAssetManager::Get().GetAsset(TargetingData);
}
