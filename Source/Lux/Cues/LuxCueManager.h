// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "LuxCueNotify.h"
#include "System/LuxCueData.h"
#include "LuxCueManager.generated.h"

class UDataTable;


USTRUCT(BlueprintType)
struct FLuxCueDataRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    /** 사용할 게임플레이 태그입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LuxCue", meta = (Categories = "Cue"))
    FGameplayTag CueTag;

    /** 해당하는 Notify 클래스입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LuxCue")
    TSubclassOf<ALuxCueNotify> CueClass;
};


USTRUCT(BlueprintType)
struct FLuxCueNotifyArray
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<TObjectPtr<ALuxCueNotify>> Cues;
};


USTRUCT(BlueprintType)
struct FActiveCueNotifyMap
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TMap<FGameplayTag, FLuxCueNotifyArray> CueMap;
};


USTRUCT(BlueprintType)
struct FLuxCueNotifyPool
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<ALuxCueNotify>> Items;
};



/**
 * 
 */
UCLASS(Blueprintable)
class LUX_API ULuxCueManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


    /**
     * 지정된 태그의 큐를 실행합니다.
     * @param Target 큐의 대상이 되는 액터입니다.
     * @param CueTag 실행할 큐의 태그입니다.
     * @param Context 큐 실행에 필요한 컨텍스트 데이터입니다.
     */
    void HandleCue(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context);

    /**
     * 지정된 태그의 큐를 중지합니다.
     * @param Target 큐의 대상이 되는 액터입니다.
     * @param CueTag 중지할 큐의 태그입니다.
	 */
    void StopCue(AActor* Target, FGameplayTag CueTag);

    /**
     * 지정된 태그에 해당하는 모든 큐를 중지합니다.
     * @param Target 큐의 대상이 되는 액터입니다.
     * @param CueTag 중지할 큐의 태그입니다.
     */
    void StopAllCuesByTag(AActor* Target, FGameplayTag CueTag);

    /**
     * 타겟에 있는 모든 큐를 중지합니다.
     * @param Target 큐의 대상이 되는 액터입니다.
     */
    void StopAllCuesForTarget(AActor* Target);


    /** 특정 액터에게 활성화된 모든 큐의 목록을 반환합니다. */
    const TMap<FGameplayTag, FLuxCueNotifyArray>* GetActiveCuesForTarget(AActor* Target) const;

    /** 특정 액터의 특정 태그에 해당하는 모든 큐 인스턴스를 반환합니다. */
    const TArray<TObjectPtr<ALuxCueNotify>>* GetCuesByTag(AActor* Target, FGameplayTag CueTag) const;

    /** 시스템 전체에서 현재 활성화된 큐 인스턴스의 총 개수를 반환합니다. */
    int32 GetTotalActiveCueCount() const;

    /** 특정 액터에게 활성화된 큐 인스턴스의 총 개수를 반환합니다. */
    int32 GetActiveCueCountForTarget(AActor* Target) const;

    /** 특정 액터의 특정 태그에 해당하는 큐가 몇 개 활성화되어 있는지 반환합니다. */
    int32 GetCueCountByTag(AActor* Target, FGameplayTag CueTag) const;

    /** 특정 액터가 특정 태그의 큐를 하나라도 가지고 있는지 확인합니다. */
    bool HasCue(AActor* Target, FGameplayTag CueTag) const;

private:
    UFUNCTION()
    void OnCueStopped(ALuxCueNotify* CueToReturn);
    void InitializeCuePool();
    ALuxCueNotify* SpawnNewCue(FGameplayTag CueTag, const FTransform& SpawnTransform);

private:
    /** Cue 태그와 실제 Notify 클래스를 매핑하는 데이터 테이블의 경로입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "LuxCue", meta = (RequiredAssetDataTags = "RowStructure=LuxCueDataRow"))
    TSoftObjectPtr<UDataTable> CueDataTable;

    /** 게임 시작 시 데이터 테이블을 읽어와서 빠른 검색용 맵입니다. */
    UPROPERTY()
    TMap<FGameplayTag, TSubclassOf<ALuxCueNotify>> CueClassMap;

    /** 현재 활성화된 모든 큐들의 목록입니다. (Target -> (CueTag -> [Cues])) */
    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, FActiveCueNotifyMap> ActiveCues;

    /** 비활성화된 큐들을 보관하는 풀입니다. */
    UPROPERTY()
    TMap<FGameplayTag, FLuxCueNotifyPool> CuePools;
};
