// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuxCueData.h"
#include "Engine/DataTable.h"
#include "System/LuxAssetManager.h"
#include "LuxLogChannels.h"

ULuxCueData::ULuxCueData()
{
	// 기본값 초기화
}

UDataTable* ULuxCueData::GetCueDataTable() const
{
	return ULuxAssetManager::Get().GetAsset(CueDataTable);
}

//UDataTable* ULuxCueData::GetSoundCueDataTable() const
//{
//	return ULuxAssetManager::Get().GetAsset(SoundCueDataTable);
//}
//
//UDataTable* ULuxCueData::GetVisualCueDataTable() const
//{
//	return ULuxAssetManager::Get().GetAsset(VisualCueDataTable);
//}
//
//UDataTable* ULuxCueData::GetParticleCueDataTable() const
//{
//	return ULuxAssetManager::Get().GetAsset(ParticleCueDataTable);
//}
//
//UDataTable* ULuxCueData::GetAnimationCueDataTable() const
//{
//	return ULuxAssetManager::Get().GetAsset(AnimationCueDataTable);
//}
