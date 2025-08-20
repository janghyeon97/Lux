// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Executions/LuxExecutionCalculation.h"
#include "LuxExecution_Damage.generated.h"

/**
 * 
 */
UCLASS()
class LUX_API ULuxExecution_Damage : public ULuxExecutionCalculation
{
	GENERATED_BODY()
	
public:
	virtual void Execute_Implementation(FLuxEffectSpec& Spec) const override;
};
