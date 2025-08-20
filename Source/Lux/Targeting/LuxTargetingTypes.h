// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "LuxTargetingTypes.generated.h"

/**
 * 타겟팅 검출 방식을 정의하는 열거형
 */
UENUM(BlueprintType)
enum class ETargetingDetectionType : uint8
{
    Overlap         UMETA(DisplayName = "Overlap Trace"),
    LineTrace       UMETA(DisplayName = "Line Trace"),
    SweepTrace      UMETA(DisplayName = "Sweep Trace")
};

/**
 * 타겟의 팀 관계를 지정하는 열거형입니다.
 */
UENUM(BlueprintType)
enum class EAttitudeQuery : uint8
{
	Any,
	Hostile,
	Friendly,
	Neutral
};
