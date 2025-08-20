// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetFilter.h"
#include "Engine/EngineTypes.h"
#include "VisibilityTargetFilter.generated.h"

/**
 * 가시성을 기반으로 타겟을 필터링하는 필터입니다.
 * 라인트레이스를 통해 시야에 가려진 타겟을 제외할 수 있습니다.
 */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Visibility Target Filter"))
class LUX_API UVisibilityTargetFilter : public UTargetFilter
{
	GENERATED_BODY()

public:
	UVisibilityTargetFilter();

	//~ UTargetFilter interface
	virtual bool IsTargetValid(AActor* Target, AActor* SourceActor) const override;
	virtual FString GetFilterDescription() const override;
	//~ End UTargetFilter interface

protected:
	/** 
	 * 가시성 확인에 사용할 트레이스 채널입니다.
	 * 일반적으로 ECC_Visibility를 사용합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter")
	TEnumAsByte<ECollisionChannel> VisibilityTraceChannel = ECC_Visibility;

	/** 
	 * 가시성 확인 시작점입니다.
	 * true: 소유자의 위치에서 시작
	 * false: 소유자의 카메라 위치에서 시작 (PlayerController가 있는 경우)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter")
	bool bUseOwnerLocation = false;

	/** 
	 * 타겟의 어느 지점을 기준으로 가시성을 확인할지 설정합니다.
	 * true: 타겟의 중심점 (GetActorLocation)
	 * false: 타겟의 루트 컴포넌트 위치
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter")
	bool bUseTargetCenter = true;

	/** 
	 * 타겟 위치에 추가할 오프셋입니다.
	 * 예: (0, 0, 50)으로 설정하면 타겟의 머리 부분을 겨냥
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter")
	FVector TargetOffset = FVector(0, 0, 50);

	/** 
	 * 복잡한 콜리전을 사용할지 여부입니다.
	 * true: 정확한 메시 콜리전 사용 (성능 저하)
	 * false: 단순한 콜리전 사용 (성능 우수)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter")
	bool bUseComplexCollision = false;

	/** 무시할 액터들입니다. (일반적으로 소유자와 타겟 자체) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter")
	TArray<AActor*> IgnoredActors;

	/** 디버그 라인을 그릴지 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter|Debug")
	bool bDrawDebugLine = false;

	/** 디버그 정보를 로그로 출력할지 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility Filter|Debug")
	bool bDebugLog = false;

private:
	/** 카메라 위치를 가져오는 헬퍼 함수입니다. */
	FVector GetCameraLocation(AActor* Actor) const;

	/** 트레이스 시작점을 가져오는 헬퍼 함수입니다. */
	FVector GetTraceStartLocation(AActor* Actor) const;
};
