#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagStack.generated.h"

class UActionSystemComponent;

struct FGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

/**
 * @struct FGameplayTagStack
 * @brief 단일 게임플레이 태그와 스택 수를 저장하며 FastArraySerializer를 통해 복제됩니다.
 */
USTRUCT(BlueprintType)
struct FGameplayTagStack : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:
    FGameplayTagStack() = default;

    FGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
        : Tag(InTag), StackCount(InStackCount)
    {
    }

    FString GetDebugString() const;

private:
    friend FGameplayTagStackContainer;

    UPROPERTY()
    FGameplayTag Tag;

    UPROPERTY()
    int32 StackCount = 0;
};

/**
 * @struct FGameplayTagStackContainer
 * @brief 스택을 지원하는 게임플레이 태그 컨테이너입니다. 
 */
USTRUCT(BlueprintType)
struct FGameplayTagStackContainer : public FFastArraySerializer
{
    GENERATED_BODY()

public:
    /** 지정된 수만큼 태그 스택을 추가합니다. (StackCount가 1 미만이면 무시) */
    void AddStack(FGameplayTag Tag, int32 StackCount);

    /** 지정된 수만큼 태그 스택을 제거합니다. (StackCount가 1 미만이면 무시) */
    void RemoveStack(FGameplayTag Tag, int32 StackCount);

    /** 지정된 태그의 현재 스택 수를 반환합니다. (없으면 0) */
    int32 GetStackCount(FGameplayTag Tag) const;

    /** 지정된 태그의 스택이 하나라도 존재하는지 확인합니다. */
    bool ContainsTag(FGameplayTag Tag) const;

    /** 컨테이너가 가진 모든 태그를 FGameplayTagContainer 형태로 반환합니다. */
    FGameplayTagContainer GetExplicitGameplayTags() const;

    /** 모든 태그 스택과 내부 캐시를 초기화합니다. (복제용 배열까지 비움) */
    void Reset();

    //~ FFastArraySerializer interface
    void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
    //~ End of FFastArraySerializer interface

public:
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FGameplayTagStack, FGameplayTagStackContainer>(Stacks, DeltaParms, *this);
    }

    UPROPERTY(Transient)
    TWeakObjectPtr<UActionSystemComponent> OwnerComponent;

private:
    /** 네트워크 복제를 위한 태그 스택 배열입니다. */
    UPROPERTY()
    TArray<FGameplayTagStack> Stacks;

    /** 빠른 조회를 위한 비복제 맵 캐시입니다. */
    TMap<FGameplayTag, int32> TagToCountMap;
};

// NetDeltaSerialize를 사용하기 위한 타입 특성(trait) 설정
template<>
struct TStructOpsTypeTraits<FGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FGameplayTagStackContainer>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};