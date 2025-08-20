// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuxCrowdControlData.h"
#include "Engine/DataTable.h"
#include "System/LuxAssetManager.h"
#include "LuxLogChannels.h"

ULuxCrowdControlData::ULuxCrowdControlData()
{
	// 기본값 초기화
}

UDataTable* ULuxCrowdControlData::GetCrowdControlTable() const
{
	return ULuxAssetManager::Get().GetAsset(CrowdControlTable);
}
