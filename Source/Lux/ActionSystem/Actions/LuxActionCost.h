// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuxActionCost.generated.h"


class UActionSystemComponent;
class UBaseGameplayEffect;
struct FLuxActionSpec;

/**
 *
 */
UCLASS()
class LUX_API ULuxActionCost : public UObject
{
	GENERATED_BODY()

public:
	/**
	  * Cost 조건을 확인합니다.
	  * @param ASC 조건을 확인할 대상의 ActionSystemComponent
	  * @param Spec 현재 사용하려는 액션의 Spec 정보
	  * @return 모든 조건을 만족하면 true를 반환합니다.
	  */
	virtual bool CheckCost(const UActionSystemComponent* ASC, const FLuxActionSpec& Spec) const
	{
		return false;
	};

	/**
	 * 비용을 실제로 적용합니다. (예: 마나 소모)
	 * @param ASC 비용을 적용할 대상의 ActionSystemComponent
	 * @param Spec 현재 사용하려는 액션의 Spec 정보
	 */
	virtual void ApplyCost(UActionSystemComponent* ASC, const FLuxActionSpec& Spec)
	{

	};
};
