#pragma once

#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "Engine/NetSerialization.h"
#include "LuxPayload.generated.h"

/** 페이로드 키 상수 */
namespace LuxPayloadKeys
{
    // === 특화된 이벤트 타입 키들 ===
    inline const FName CrowdControlEvent(TEXT("CrowdControlEvent"));
    inline const FName DamageEvent(TEXT("DamageEvent"));
    inline const FName HealingEvent(TEXT("HealingEvent"));
    inline const FName StatusEffectEvent(TEXT("StatusEffectEvent"));
    
    // === 범용 이벤트 키 (하위 호환성) ===
    inline const FName GameplayEventData(TEXT("GameplayEventData"));
    
    // === 기타 페이로드 키들 ===
    inline const FName ActionSpec(TEXT("ActionSpec"));
    inline const FName ActionTags(TEXT("ActionTags"));
    inline const FName Level(TEXT("Level"));
    inline const FName HitData(TEXT("HitData"));
    inline const FName PathData(TEXT("PathData"));
    inline const FName NotifyName(TEXT("NotifyName"));
    inline const FName TargetingData(TEXT("TargetingData"));
    
    // === 공간 정보 키들 ===
    inline const FName Location(TEXT("Location"));
    inline const FName Rotation(TEXT("Rotation"));
    inline const FName Direction(TEXT("Direction"));
    
    // === 레거시 키들 (새 코드에서는 특화된 구조체 사용 권장) ===
    inline const FName Damage(TEXT("Damage"));
}

// ==================== 기본 페이로드 구조체들 ====================

/**
 * 모든 페이로드 구조체의 베이스 클래스
 * 공통적인 NetSerialize 인터페이스를 제공합니다.
 */
USTRUCT(BlueprintType)
struct FPayload
{
    GENERATED_BODY()

    /** 가상 소멸자 */
    virtual ~FPayload() = default;

    /** NetSerialize 가상 함수 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

// ==================== 기본 타입 페이로드들 ====================

/** FName 타입의 데이터를 담기 위한 범용 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_Name : public FPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName Value = NAME_None;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/** FGameplayTag 타입의 데이터를 담기 위한 범용 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_Tag : public FPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag Value;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/** FGameplayTagContainer 타입의 데이터를 담기 위한 범용 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_TagContainer : public FPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGameplayTagContainer Value;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/** float 타입의 데이터를 담기 위한 범용 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_Float : public FPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float Value = 0.0f;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/** FVector 타입의 데이터를 담기 위한 범용 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_Vector : public FPayload
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    FVector Value = FVector::ZeroVector;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/** FRotator 타입의 데이터를 담기 위한 범용 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_Rotator : public FPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FRotator Value = FRotator::ZeroRotator;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

// ==================== 특화된 데이터 페이로드들 ====================

/** 경로 데이터를 담기 위한 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_PathData : public FPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> PathPoints;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/** 피해 데이터를 담기 위한 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_Damage : public FPayload
{
    GENERATED_BODY()

    /** 최종 피해량 (양수 값) */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Damage")
    float Amount = 0.0f;

    /** 치명타 여부 */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Damage")
    bool bIsCritical = false;

    /** 피해 타입들 (물리/마법 등) */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Damage")
    FGameplayTagContainer DamageTypes;

    /** 이 피해를 발생시킨 액션의 식별 태그 (없을 수 있음) */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Damage")
    FGameplayTag SourceActionIdentifierTag;

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/** 타겟 히트 데이터를 담기 위한 페이로드 구조체 */
USTRUCT(BlueprintType)
struct FPayload_HitData : public FPayload
{
    GENERATED_BODY()

    /** 타겟 액터입니다. 네트워크를 통해 WeakObjectPtr이 복제됩니다. */
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    // 타겟 액터만 설정합니다.
    void SetData(AActor* Target);

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

// ==================== 게임플레이 이벤트 페이로드들 ====================

/**
 * 범용 게임플레이 이벤트 데이터 - 모든 이벤트의 베이스 구조체
 * 모든 게임플레이 이벤트에서 공통적으로 사용되는 기본 메타데이터만 포함
 * 특화된 이벤트들은 이 구조체를 상속받아 추가 필드를 정의
 */
USTRUCT(BlueprintType)
struct FPayload_GameplayEventData : public FPayload
{
    GENERATED_BODY()

public:
    FPayload_GameplayEventData() = default;
    FPayload_GameplayEventData(const FPayload_GameplayEventData&) = default;
    FPayload_GameplayEventData& operator=(const FPayload_GameplayEventData&) = default;
    FPayload_GameplayEventData(FPayload_GameplayEventData&&) = default;
	FPayload_GameplayEventData& operator=(FPayload_GameplayEventData&&) = default;

	virtual ~FPayload_GameplayEventData() = default;

    /** 이벤트가 유효한지 기본 검사 */
    virtual bool IsValidEvent() const
    {
        return Instigator.IsValid() || Target.IsValid();
    }

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

public:
    /** 이벤트를 발생시킨 액터 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EventData")
    TWeakObjectPtr<AActor> Instigator;

    /** 이벤트 대상 액터 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EventData")
    TWeakObjectPtr<AActor> Target;

    /** 이벤트가 발생한 월드 위치 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EventData")
    FVector_NetQuantize EventLocation;

    /** 이벤트의 방향 (방향성이 있는 이벤트의 경우) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EventData")
    FVector_NetQuantizeNormal EventDirection;

    /** 이벤트 발생 시간 (게임 시간) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EventData")
    float EventTime = 0.0f;

    /** 이벤트의 레벨/등급/강도 (레벨 기반 시스템에서 공통 사용) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EventData")
    int32 EventLevel = 1;
};

/**
 * CrowdControl 이벤트 전용 데이터
 */
USTRUCT(BlueprintType)
struct FPayload_CrowdControlEvent : public FPayload_GameplayEventData
{
    GENERATED_BODY()

    /** CC 타입 태그 (예: Stun, Snare 등) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CrowdControl")
    FGameplayTag Type;

    /** 지속 시간(초) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CrowdControl")
    float Duration = 0.0f;

    /** 강도 또는 크기 (의미는 CC별로 다름) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CrowdControl")
    float Magnitude = 0.0f;

    /** 데이터가 유효한지 확인 */
    bool IsValid() const
    {
        return Type.IsValid() && Duration > 0.0f;
    }

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/**
 * Damage 이벤트 전용 데이터
 */
USTRUCT(BlueprintType)
struct FPayload_DamageEvent : public FPayload_GameplayEventData
{
    GENERATED_BODY()

    /** 최종 피해량 (양수 값) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
    float Amount = 0.0f;

    /** 치명타 여부 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
    bool bIsCritical = false;

    /** 피해 타입들 (물리/마법 등) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
    FGameplayTagContainer DamageTypes;

    /** 이 피해를 발생시킨 액션의 고유 식별 태그 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
    FGameplayTag SourceActionTag;

    /** 이 피해를 발생시킨 액션의 타입 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
    FGameplayTag SourceActionTypeTag;

    /** 데이터가 유효한지 확인 */
    bool IsValid() const
    {
        return Amount > 0.0f;
    }

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/**
 * Healing 이벤트 전용 데이터  
 */
USTRUCT(BlueprintType)
struct FPayload_HealingEvent : public FPayload_GameplayEventData
{
    GENERATED_BODY()

    /** 회복량 (양수 값) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Healing")
    float Amount = 0.0f;

    /** 회복 타입들 (즉시/지속 등) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Healing")
    FGameplayTagContainer HealingTypes;

    /** 이 회복을 발생시킨 액션의 식별 태그 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Healing")
    FGameplayTag SourceActionTag;

    /** 데이터가 유효한지 확인 */
    bool IsValid() const
    {
        return Amount > 0.0f;
    }

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

/**
 * 상태 효과(Buff/Debuff) 이벤트 전용 데이터
 */
USTRUCT(BlueprintType)
struct FPayload_StatusEffectEvent : public FPayload_GameplayEventData
{
    GENERATED_BODY()

    /** 상태 효과 타입 태그 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StatusEffect")
    FGameplayTag EffectType;

    /** 지속 시간(초), 0이면 영구 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StatusEffect")
    float Duration = 0.0f;

    /** 스택 수 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StatusEffect")
    int32 StackCount = 1;

    /** 강도 또는 크기 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StatusEffect")
    float Magnitude = 0.0f;

    /** 이 효과를 발생시킨 액션의 식별 태그 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StatusEffect")
    FGameplayTag SourceActionTag;

    /** 데이터가 유효한지 확인 */
    bool IsValid() const
    {
        return EffectType.IsValid();
    }

    /** NetSerialize 구현 */
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

// ==================== 컨텍스트 페이로드 ====================

/**
 * @struct FContextPayload
 * @brief 액션 시스템 전체에서 사용되는 통합 페이로드 구조체입니다.
 * @details 페이즈 간 데이터 전달, 태스크-액션 간 이벤트 데이터 전달 등
 * 모든 종류의 데이터 교환을 위한 표준 컨테이너 역할을 합니다.
 */
USTRUCT(BlueprintType)
struct FContextPayload
{
    GENERATED_BODY()

    friend class ULuxAction;

private:
    // TMap 대신 배열 기반 구조 사용
    UPROPERTY()
    TArray<FInstancedStruct> DataArray;

    // 키-값 쌍을 위한 별도 배열
    UPROPERTY()
    TArray<FName> DataKeys;

public:
    /** 페이로드에 특정 구조체 데이터를 저장합니다. */
    template <typename T>
    void SetData(const FName Key, const T& StructData)
    {
        // 기존 키가 있으면 업데이트, 없으면 추가
        int32 Index = DataKeys.Find(Key);
        if (Index != INDEX_NONE)
        {
            DataArray[Index].InitializeAs<T>(StructData);
        }
        else
        {
            DataKeys.Add(Key);
            FInstancedStruct NewStruct;
            NewStruct.InitializeAs<T>(StructData);
            DataArray.Add(NewStruct);
        }
    }

    /** 페이로드에서 특정 구조체 데이터를 가져옵니다. (읽기 전용) */
    template <typename T>
    const T* GetData(const FName Key) const
    {
        int32 Index = DataKeys.Find(Key);
        if (Index != INDEX_NONE && DataArray.IsValidIndex(Index))
        {
            return DataArray[Index].GetPtr<T>();
        }
        return nullptr;
    }

    /** 특정 키의 데이터가 존재하는지 확인합니다. */
    bool HasData(const FName Key) const
    {
        return DataKeys.Contains(Key);
    }

    /** 모든 데이터를 제거합니다. */
    void Clear()
    {
        DataArray.Empty();
        DataKeys.Empty();
    }

    /** 데이터 개수를 반환합니다. */
    int32 Num() const
    {
        return DataArray.Num();
    }

    /** 배열 데이터를 직렬화/역직렬화하는 함수입니다. */
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

// ==================== TStructOpsTypeTraits 등록 ====================

/**
 * FPayload의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload> : public TStructOpsTypeTraitsBase2<FPayload>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_Name의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_Name> : public TStructOpsTypeTraitsBase2<FPayload_Name>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_Tag의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_Tag> : public TStructOpsTypeTraitsBase2<FPayload_Tag>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_TagContainer의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_TagContainer> : public TStructOpsTypeTraitsBase2<FPayload_TagContainer>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_Float의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_Float> : public TStructOpsTypeTraitsBase2<FPayload_Float>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_Vector의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_Vector> : public TStructOpsTypeTraitsBase2<FPayload_Vector>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_Rotator의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_Rotator> : public TStructOpsTypeTraitsBase2<FPayload_Rotator>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_PathData의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_PathData> : public TStructOpsTypeTraitsBase2<FPayload_PathData>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_Damage의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_Damage> : public TStructOpsTypeTraitsBase2<FPayload_Damage>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_HitData의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_HitData> : public TStructOpsTypeTraitsBase2<FPayload_HitData>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_GameplayEventData의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_GameplayEventData> : public TStructOpsTypeTraitsBase2<FPayload_GameplayEventData>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_CrowdControlEvent의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_CrowdControlEvent> : public TStructOpsTypeTraitsBase2<FPayload_CrowdControlEvent>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_DamageEvent의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_DamageEvent> : public TStructOpsTypeTraitsBase2<FPayload_DamageEvent>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_HealingEvent의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_HealingEvent> : public TStructOpsTypeTraitsBase2<FPayload_HealingEvent>
{
    enum { WithNetSerializer = true };
};

/**
 * FPayload_StatusEffectEvent의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FPayload_StatusEffectEvent> : public TStructOpsTypeTraitsBase2<FPayload_StatusEffectEvent>
{
    enum { WithNetSerializer = true };
};

/**
 * FContextPayload의 NetSerializer 등록
 */
template<>
struct TStructOpsTypeTraits<FContextPayload> : public TStructOpsTypeTraitsBase2<FContextPayload>
{
    enum { WithNetSerializer = true };
};