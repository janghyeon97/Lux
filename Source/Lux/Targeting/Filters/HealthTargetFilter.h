// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetFilter.h"
#include "HealthTargetFilter.generated.h"

UENUM(BlueprintType)
enum class EHealthFilterMode : uint8
{
	/** 살아있는 타겟만 허용 (체력 > 0) */
	AliveOnly,
	
	/** 죽은 타겟만 허용 (체력 <= 0) */
	DeadOnly,
	
	/** 특정 체력 범위 내의 타겟만 허용 */
	HealthRange,
	
	/** 특정 체력 비율 범위 내의 타겟만 허용 (0.0 ~ 1.0) */
	HealthPercentageRange,
	
	/** 낮은 체력의 타겟만 허용 (체력 비율이 임계값 이하) */
	LowHealth
};

/**
 * 체력을 기반으로 타겟을 필터링하는 필터입니다.
 * 살아있는 타겟만 허용하거나, 특정 체력 범위의 타겟만 허용할 수 있습니다.
 */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Health Target Filter"))
class LUX_API UHealthTargetFilter : public UTargetFilter
{
	GENERATED_BODY()

public:
	UHealthTargetFilter();

	//~ UTargetFilter interface
	virtual bool IsTargetValid(AActor* Target, AActor* SourceActor) const override;
	virtual FString GetFilterDescription() const override;
	//~ End UTargetFilter interface

protected:
	/** 체력 필터링 모드입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Filter")
	EHealthFilterMode FilterMode = EHealthFilterMode::AliveOnly;

	/** 허용할 최소 체력입니다. (HealthRange 모드에서 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Filter", 
		meta = (EditCondition = "FilterMode == EHealthFilterMode::HealthRange", ClampMin = "0"))
	float MinHealth = 0.0f;

	/** 허용할 최대 체력입니다. (HealthRange 모드에서 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Filter", 
		meta = (EditCondition = "FilterMode == EHealthFilterMode::HealthRange", ClampMin = "0"))
	float MaxHealth = 100.0f;

	/** 허용할 최소 체력 비율입니다. (HealthPercentageRange 모드에서 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Filter", 
		meta = (EditCondition = "FilterMode == EHealthFilterMode::HealthPercentageRange", ClampMin = "0.0", ClampMax = "1.0"))
	float MinHealthPercentage = 0.0f;

	/** 허용할 최대 체력 비율입니다. (HealthPercentageRange 모드에서 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Filter", 
		meta = (EditCondition = "FilterMode == EHealthFilterMode::HealthPercentageRange", ClampMin = "0.0", ClampMax = "1.0"))
	float MaxHealthPercentage = 1.0f;

	/** 낮은 체력으로 간주할 임계값입니다. (LowHealth 모드에서 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Filter", 
		meta = (EditCondition = "FilterMode == EHealthFilterMode::LowHealth", ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.3f;

	/** 디버그 정보를 로그로 출력할지 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Filter|Debug")
	bool bDebugLog = false;

private:
	/** 타겟의 현재 체력을 가져오는 헬퍼 함수입니다. */
	float GetTargetCurrentHealth(AActor* Target) const;

	/** 타겟의 최대 체력을 가져오는 헬퍼 함수입니다. */
	float GetTargetMaxHealth(AActor* Target) const;

	/** 타겟의 체력 비율을 가져오는 헬퍼 함수입니다. */
	float GetTargetHealthPercentage(AActor* Target) const;
};
