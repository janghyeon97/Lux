// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetFilter.h"
#include "DistanceTargetFilter.generated.h"

/**
 * 거리를 기반으로 타겟을 필터링하는 필터입니다.
 * 최소/최대 거리 범위를 설정하여 범위 내의 타겟만 허용할 수 있습니다.
 */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Distance Target Filter"))
class LUX_API UDistanceTargetFilter : public UTargetFilter
{
	GENERATED_BODY()

public:
	UDistanceTargetFilter();

	//~ UTargetFilter interface
	virtual bool IsTargetValid(AActor* Target, AActor* SourceActor) const override;
	virtual FString GetFilterDescription() const override;
	//~ End UTargetFilter interface

protected:
	/** 허용할 최소 거리입니다. (0 이하면 제한 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Filter", meta = (ClampMin = "0"))
	float MinDistance = 0.0f;

	/** 허용할 최대 거리입니다. (0 이하면 제한 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Filter", meta = (ClampMin = "0"))
	float MaxDistance = 1000.0f;

	/** 
	 * 거리 계산에 사용할 기준점입니다.
	 * true: 소유자의 위치 기준
	 * false: 소유자의 카메라 위치 기준 (PlayerController가 있는 경우)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Filter")
	bool bUseOwnerLocation = true;

	/** 2D 거리만 고려할지 여부입니다. (Z축 무시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Filter")
	bool bUse2DDistance = false;

	/** 디버그 정보를 로그로 출력할지 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Filter|Debug")
	bool bDebugLog = false;

private:
	/** 카메라 위치를 가져오는 헬퍼 함수입니다. */
	FVector GetCameraLocation(AActor* Actor) const;
};
