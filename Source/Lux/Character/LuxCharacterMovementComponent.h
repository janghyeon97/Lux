// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LuxCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class LUX_API ULuxCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** 캐릭터의 상태(예: 태그)를 모두 고려하여 최종 최대 속도를 반환합니다. */
	virtual float GetMaxSpeed() const override;
};
