// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Camera/LuxCameraMode.h"
#include "LuxPawnData.generated.h"


class APawn;
class ULuxCameraMode;
class ULuxActionSet;
class ULuxAnimationData;
class ULuxActionTagRelationshipMapping;
class ULuxInputConfig;
class UObject;


/**
 * ULuxPawnData
 *
 * 변경 불가능한(non-mutable) 폰 정의 속성을 담고 있는 데이터 에셋입니다.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "LuxPawnData"))
class LUX_API ULuxPawnData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    ULuxPawnData(const FObjectInitializer& ObjectInitializer);

public:
    // 이 폰을 인스턴스화할 클래스입니다 (보통 ALuxPawn 또는 ALuxCharacter에서 파생됩니다).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|Pawn")
    TSubclassOf<APawn> PawnClass;

    // 이 폰의 어빌리티 시스템에 부여할 Ability Set들입니다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|Abilities")
    TArray<TObjectPtr<ULuxActionSet>> ActionSets;

    // 이 폰의 액션에 사용할 어빌리티 태그 매핑입니다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|Abilities")
    TObjectPtr<ULuxActionTagRelationshipMapping> TagRelationshipMapping;

    // 플레이어 제어 폰의 입력 매핑 생성 및 입력 액션 바인딩에 사용할 입력 구성입니다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|Input")
    TObjectPtr<ULuxInputConfig> InputConfig;

    /** 폰이 사용하는 미리 로드해야 할 액터 클래스 목록입니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|Preloading", meta = (AssetBundles = "Client,Server"))
    TArray<TSoftClassPtr<AActor>> PreloadedActorClasses;

    /** 폰의 기본 카메라 모드입니다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|Camera")
    TSubclassOf<ULuxCameraMode> DefaultCameraMode;
};