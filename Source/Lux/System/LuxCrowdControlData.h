// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "LuxCrowdControlData.generated.h"

class UDataTable;
class UObject;

/**
 * 게임에서 사용되는 크라우드 컨트롤 관련 데이터들을 관리하는 데이터 에셋입니다.
 * 크라우드 컨트롤 액션 매핑, 효과 정의 등과 관련된 데이터를 포함합니다.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Lux Crowd Control Data", ShortTooltip = "Data asset containing crowd control related data."))
class LUX_API ULuxCrowdControlData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULuxCrowdControlData();

public:
	/** 크라우드 컨트롤 액션 매핑 테이블입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Crowd Control Actions")
	TSoftObjectPtr<UDataTable> CrowdControlTable;

	// 편의 함수들
	UFUNCTION(BlueprintPure, Category = "Crowd Control Data Access")
	UDataTable* GetCrowdControlTable() const;
};
