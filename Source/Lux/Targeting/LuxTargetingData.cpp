// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuxTargetingData.h"
#include "LuxLogChannels.h"

ULuxTargetingData::ULuxTargetingData()
{
	// 기본값 초기화 (TargetingComponent와 동일)
	TraceDistance = 1000.0f;
	OverlapRadius = 2500.0f;
	SweepRadius = 50.0f;
	DetectionType = ETargetingDetectionType::LineTrace;
	bEnablePrioritySelection = true;
	bEnableTargeting = true;
	DistanceWeight = 0.5f;
	AngleWeight = 0.3f;
	SizeWeight = 0.2f;
	bDrawDebugTrace = true;
	bEnableOverlayMaterial = true;
	HostileOverlayMaterial = nullptr;
	FriendlyOverlayMaterial = nullptr;
	NeutralOverlayMaterial = nullptr;
}

bool ULuxTargetingData::IsTargetInRange(float Distance) const
{
	// 간단한 거리 체크
	return Distance <= TraceDistance;
}

bool ULuxTargetingData::IsTargetInAngle(float Angle) const
{
	// 간단한 각도 체크 (90도 이내)
	return FMath::Abs(Angle) <= 90.0f;
}

float ULuxTargetingData::CalculateTargetPriority(float Distance, float Angle, float Size) const
{
	if (!bEnablePrioritySelection)
	{
		return 0.0f;
	}

	float Priority = 0.0f;
	
	// 거리 기반 우선순위 (가까울수록 높은 우선순위)
	if (DistanceWeight > 0.0f)
	{
		float DistanceScore = FMath::Max(0.0f, 1.0f - (Distance / TraceDistance));
		Priority += DistanceScore * DistanceWeight;
	}
	
	// 각도 기반 우선순위 (중앙에 가까울수록 높은 우선순위)
	if (AngleWeight > 0.0f)
	{
		float AngleScore = FMath::Max(0.0f, 1.0f - (FMath::Abs(Angle) / 90.0f));
		Priority += AngleScore * AngleWeight;
	}
	
	// 크기 기반 우선순위 (클수록 높은 우선순위)
	if (SizeWeight > 0.0f)
	{
		float SizeScore = FMath::Clamp(Size / 100.0f, 0.0f, 1.0f); // 크기를 0-1 범위로 정규화
		Priority += SizeScore * SizeWeight;
	}
	
	return Priority;
}
