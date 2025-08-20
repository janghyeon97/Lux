// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TargetFilter.generated.h"

class AActor;
struct FHitResult;

/**
 * 타겟팅 시스템에서 사용되는 필터의 기본 클래스입니다.
 * 특정 조건에 따라 타겟을 필터링하는 기능을 제공합니다.
 */
UCLASS(Blueprintable, BlueprintType)
class LUX_API UTargetFilter : public UObject
{
	GENERATED_BODY()

public:
	UTargetFilter();

    /**
     * 필터의 조건에 따라 타겟이 유효한지 판단합니다.
     * @param Target 필터링할 타겟 액터
     * @param SourceActor 타겟팅을 시도하는 액터 (소유자)
     * @return 타겟이 유효하면 true를 반환합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "Targeting")
    virtual bool IsTargetValid(AActor* Target, AActor* SourceActor) const
    {
        return true; // 기본적으로는 항상 통과
    }

	/**
	 * 필터의 이름을 반환합니다. 디버깅 및 로깅에 사용됩니다.
	 * @return 필터의 이름
	 */
	UFUNCTION(BlueprintPure, Category = "Targeting|Filter")
	virtual FString GetFilterName() const { return GetClass()->GetName(); }

	/**
	 * 필터의 이름을 반환합니다. 디버깅 및 로깅에 사용됩니다.
	 * @return 필터의 이름
	 */
	UFUNCTION(BlueprintPure, Category = "Targeting|Filter")
	virtual FString GetFilterDescription() const { return FString(); }
}; 