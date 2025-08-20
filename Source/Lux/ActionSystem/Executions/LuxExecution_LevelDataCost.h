// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Executions/LuxExecutionCalculation.h"
#include "LuxExecution_LevelDataCost.generated.h"

/**
 * 액션의 레벨 데이터 테이블에 정의된 고정 Cost(비용) 값을 적용하는 Execution입니다.
 */
UCLASS()
class LUX_API ULuxExecution_LevelDataCost : public ULuxExecutionCalculation
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FLuxEffectSpec& Spec) const override;
};
