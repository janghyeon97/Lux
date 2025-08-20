// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "InstancedStruct.h"
#include "LuxActionLevelData.generated.h"

class ULuxEffect;

/**
 * 모든 액션 레벨 데이터가 상속받는 기본 구조체입니다.
 * 데미지, 비용, 쿨다운 등 공통 데이터를 정의합니다.
 */
USTRUCT(BlueprintType)
struct FActionLevelDataBase
{
    GENERATED_BODY()

public:
    /** 기본 물리 피해량 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float PhysicalDamage = 0.f;

    /** 기본 마법 피해량 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float MagicalDamage = 0.f;

    /** 기본 물리 패해량 계수 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float PhysicalScale = 1.f;

    /** 기본 마법 피해량 계수 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float MagicalScale = 1.f;

    /** 소모되는 자원의 양 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float Cost = 0.f;

    /** 재사용 대기시간 (초 단위) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float Cooldown = 0.f;

    /** 재사용 가능한 최대 횟수입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData", meta = (ClampMin = "1"))
    int32 MaxMultiCastCount = 3;

    /** 다음 재사용 입력을 기다리는 시간입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData", meta = (ClampMin = "0.1"))
    float RecastWindowDuration = 3.0f;

    /** 최대 충전 가능 횟수입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData", meta = (ClampMin = "1"))
    int32 MaxChargeStacks = 1;

    /** 시전 사거리 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float Range = 0.f;

    /** 효과가 지속되는 시간 (도트 데미지, 버프/디버프 등) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float Duration = 0.f;

    /** 효과가 발생하는 주기 (도트 데미지, 버프/디버프 등) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float Period = 0.f;

    /** 발사되는 투사체의 개수, 생성되는 오브젝트의 수, 최대 타격 횟수 등 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    int32 Count = 1;

    /** 광역 효과 반경 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float Radius = 0.f;

    /** 실행 간격. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    float Rate = 0.01f;
};


USTRUCT(BlueprintType)
struct LUX_API FLuxActionLevelData : public FTableRowBase
{
    GENERATED_BODY()

public:

    /** 각 액션에 필요한 고유 데이터를 담는 컨테이너입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData", meta = (BaseStruct = "/Script/Lux.ActionLevelDataBase"))
    FInstancedStruct ActionSpecificData;

    /** 특별히 추가되거나 변경되는 효과 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelData")
    TSubclassOf<ULuxEffect> SpecialEffectToAdd;
};