// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Executions/LuxExecutionCalculation.h"
#include "LuxExecution_AttackSpeedCooldown.generated.h"

/**
 * 시전자의 공격 속도(AttackSpeed)에 기반하여 쿨다운(공격 주기)을 계산하는 Execution입니다.
 */
UCLASS()
class LUX_API ULuxExecution_AttackSpeedCooldown : public ULuxExecutionCalculation
{
	GENERATED_BODY()
	
public:
	virtual void Execute_Implementation(FLuxEffectSpec& Spec) const override;
};
