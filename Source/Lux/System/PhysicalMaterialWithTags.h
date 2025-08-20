// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "PhysicalMaterialWithTags.generated.h"

class UObject;


/**
 * UPhysicalMaterialWithTags
 *
 * UPhysicalMaterial�� Ȯ���Ͽ� FGameplayTagContainer Tags�� �����մϴ�.
 * ǥ�� ����(�ݼ�, ����, ���� ��)�� �±׷� ������ �浹, ��ƼŬ, ���� ���� �� ���� ����� �� �ֽ��ϴ�.
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
