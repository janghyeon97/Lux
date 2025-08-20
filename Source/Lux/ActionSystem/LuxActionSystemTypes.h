#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameplayTagContainer.h"
#include "LuxActionSystemTypes.generated.h"



class AActor;
class UActionSystemComponent;
class APlayerController;

USTRUCT(BlueprintType)
struct FOwningActorInfo
{
    GENERATED_BODY()

public:
    FOwningActorInfo() = default;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

    /** 동일한지 비교하는 연산자입니다. */
    bool operator==(const FOwningActorInfo& Other) const;

    /** 다른지 비교하는 연산자입니다. */
    bool operator!=(const FOwningActorInfo& Other) const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> OwnerActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> AvatarActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<UActionSystemComponent> ActionSystemComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AController> Controller;

    // 필요에 따라 다른 정보 추가 (예: 무기, 타겟 등)
};

// 예측 키 구조체
USTRUCT()
struct FLuxPredictionKey
{
    GENERATED_BODY()

public:
    bool IsValidKey() { return Key > 0; };

    bool operator==(const FLuxPredictionKey& Other) const
    {
        return Key == Other.Key;
    };

    friend uint32 GetTypeHash(const FLuxPredictionKey& InKey)
    {
        return GetTypeHash(InKey.Key);
    }

public:
    UPROPERTY(EditDefaultsOnly)
    int32 Key = 0;
};


template<>
struct TStructOpsTypeTraits<FOwningActorInfo> : public TStructOpsTypeTraitsBase2<FOwningActorInfo>
{
    enum { WithNetSerializer = true };
};