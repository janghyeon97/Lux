// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "LuxTargetingTypes.h"
#include "LuxTargetingData.generated.h"

class UObject;
class UMaterialInterface;

/**
 * 게임에서 사용되는 타겟팅 관련 데이터들을 관리하는 데이터 에셋입니다.
 * 우선순위 가중치, 검출 범위 등과 관련된 데이터를 포함합니다.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Lux Targeting Data", ShortTooltip = "Data asset containing targeting related data."))
class LUX_API ULuxTargetingData : public UDataAsset
{
	GENERATED_BODY()

public:
	ULuxTargetingData();

public:
	// TargetingComponent와 동일한 변수들 (오버라이드 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Settings")
	float TraceDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Settings")
	float OverlapRadius = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Settings")
	float SweepRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Settings")
	ETargetingDetectionType DetectionType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Settings")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Priority")
	bool bEnablePrioritySelection = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Settings")
	bool bEnableTargeting = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Priority")
	float DistanceWeight = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Priority")
	float AngleWeight = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Priority")
	float SizeWeight = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	bool bDrawDebug = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	bool bDrawDebugPriority = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	bool bDrawDebugTrace = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Overlay")
	bool bEnableOverlayMaterial = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Overlay")
	UMaterialInterface* HostileOverlayMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Overlay")
	UMaterialInterface* FriendlyOverlayMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Overlay")
	UMaterialInterface* NeutralOverlayMaterial = nullptr;

	// 타겟 업데이트 성능 및 안정성 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Performance")
	float TargetUpdateInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Stability")
	float TargetChangeThreshold = 50.0f;

	// 편의 함수들
	UFUNCTION(BlueprintPure, Category = "Targeting Data Access")
	bool IsTargetInRange(float Distance) const;

	UFUNCTION(BlueprintPure, Category = "Targeting Data Access")
	bool IsTargetInAngle(float Angle) const;

	UFUNCTION(BlueprintPure, Category = "Targeting Data Access")
	float CalculateTargetPriority(float Distance, float Angle, float Size) const;
};
