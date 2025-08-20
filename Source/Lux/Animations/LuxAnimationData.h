// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LuxAnimationData.generated.h"




/**
 * @struct FStanceData
 * @brief 단일 자세(Stance)에 대한 정보와 그 설명을 담는 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FStanceData
{
	GENERATED_BODY()

	/** 이 자세를 나타내는 게임플레이 태그입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag StanceTag;

	/** 디자이너가 참고할 수 있도록 이 자세에 대한 간단한 설명입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Description;
};

/**
 * 
 */
UCLASS()
class LUX_API ULuxAnimationData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/**
	 * @brief AnimInstance가 사용할 자세(Stance)의 우선순위 목록입니다.
	 * 배열의 맨 위(인덱스 0)에 있는 자세가 가장 높은 우선순위를 가집니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TArray<FStanceData> StancePriorityList;
};
