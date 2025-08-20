// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuxExecutionCalculation.generated.h"

struct FLuxEffectSpec;

/**
 * 
 */
UCLASS()
class LUX_API ULuxExecutionCalculation : public UObject
{
	GENERATED_BODY()
	
public:
    /** 이펙트 스펙을 기반으로 최종 수치를 계산하기 위해 호출됩니다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Execution")
    void Execute(UPARAM(ref) FLuxEffectSpec& Spec) const;

    virtual void Execute_Implementation(FLuxEffectSpec& Spec) const;
};
