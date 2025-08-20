// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "PhysicalMaterialWithTags.generated.h"

class UObject;


/**
 * UPhysicalMaterialWithTags
 *
 * UPhysicalMaterial을 확장하여 FGameplayTagContainer Tags를 포함합니다.
 * 표면 유형(금속, 나무, 얼음 등)을 태그로 지정해 충돌, 파티클, 사운드 로직 등 에서 사용할 수 있습니다.
 */
UCLASS()
class UPhysicalMaterialWithTags : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	UPhysicalMaterialWithTags(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=PhysicalProperties)
	FGameplayTagContainer Tags;
};
