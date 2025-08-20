// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LuxTargetingInterface.generated.h"

class UTargetingComponent;

UINTERFACE(MinimalAPI, Blueprintable)
class ULuxTargetingInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LUX_API ILuxTargetingInterface
{
	GENERATED_BODY()

public:
	/**
	 * 엑터에 존재하는 TargetingComponent 를 반환합니다.
	 * @return TargetingComponent, 없으면 nullptr.
	 */
	virtual UTargetingComponent* GetTargetingComponent() const = 0;
};
