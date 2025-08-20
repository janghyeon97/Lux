// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "LuxGameData.generated.h"


class ULuxEffectData;
class ULuxCueData;
class ULuxCrowdControlData;
class ULuxTargetingData;
class UObject;
class UDataTable;




UCLASS(BlueprintType, Const, Meta = (DisplayName = "Lux Game Data", ShortTooltip = "Data asset containing global game data."))
class ULuxGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULuxGameData();

	static const ULuxGameData& Get();

public:
	/** EffectData를 로드하여 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Game Data")
	ULuxEffectData* GetEffectData() const;

	/** CueData를 로드하여 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Game Data")
	ULuxCueData* GetCueData() const;

	/** CrowdControlData를 로드하여 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Game Data")
	ULuxCrowdControlData* GetCrowdControlData() const;

	/** TargetingData를 로드하여 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Game Data")
	ULuxTargetingData* GetTargetingData() const;

public:
	/** 이펙트 데이터 에셋 */
	UPROPERTY(EditDefaultsOnly, Category = "Data Assets")
	TSoftObjectPtr<ULuxEffectData> EffectData;

	/** 큐 데이터 에셋 */
	UPROPERTY(EditDefaultsOnly, Category = "Data Assets")
	TSoftObjectPtr<ULuxCueData> CueData;

	/** 크라우드 컨트롤 데이터 에셋 */
	UPROPERTY(EditDefaultsOnly, Category = "Data Assets")
	TSoftObjectPtr<ULuxCrowdControlData> CrowdControlData;

	/** 타겟팅 데이터 에셋 */
	UPROPERTY(EditDefaultsOnly, Category = "Data Assets")
	TSoftObjectPtr<ULuxTargetingData> TargetingData;
};
