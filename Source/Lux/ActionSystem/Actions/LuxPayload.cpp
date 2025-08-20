// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Actions/LuxPayload.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "UObject/UObjectGlobals.h"

// ==================== 기본 페이로드 NetSerialize 구현 ====================

/**
 * @brief FPayload 베이스 클래스의 NetSerialize 구현
 * @details 기본적으로는 아무것도 직렬화하지 않습니다.
 *          하위 클래스에서 override하여 실제 직렬화를 구현합니다.
 */
bool FPayload::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    return bOutSuccess;
}

// ==================== 기본 타입 페이로드 NetSerialize 구현 ====================

/**
 * @brief FPayload_Name의 NetSerialize 구현
 */
bool FPayload_Name::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Ar << Value;
    return bOutSuccess;
}

/**
 * @brief FPayload_Tag의 NetSerialize 구현
 */
bool FPayload_Tag::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Value.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    return bOutSuccess;
}

/**
 * @brief FPayload_TagContainer의 NetSerialize 구현
 */
bool FPayload_TagContainer::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Value.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    return bOutSuccess;
}

/**
 * @brief FPayload_Float의 NetSerialize 구현
 */
bool FPayload_Float::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Ar << Value;
    return bOutSuccess;
}

/**
 * @brief FPayload_Vector의 NetSerialize 구현
 */
bool FPayload_Vector::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Ar << Value;
    return bOutSuccess;
}

/**
 * @brief FPayload_Rotator의 NetSerialize 구현
 */
bool FPayload_Rotator::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Ar << Value;
    return bOutSuccess;
}

// ==================== 특화된 데이터 페이로드 NetSerialize 구현 ====================

/**
 * @brief FPayload_PathData의 NetSerialize 구현
 */
bool FPayload_PathData::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Ar << PathPoints;
    return bOutSuccess;
}

/**
 * @brief FPayload_Damage의 NetSerialize 구현
 */
bool FPayload_Damage::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Ar << Amount;
    Ar << bIsCritical;
    
    DamageTypes.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    SourceActionIdentifierTag.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    return bOutSuccess;
}

void FPayload_HitData::SetData(AActor* Target)
{
    TargetActor = Target;
}

/**
 * @brief FPayload_HitData의 NetSerialize 구현
 */
bool FPayload_HitData::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;

    uint8 RepBits = 0;
    if (Ar.IsSaving())
    {
        if (TargetActor.IsValid())
        {
            RepBits |= 1 << 0;
        }
    }

    Ar.SerializeBits(&RepBits, 5);

    if (RepBits & (1 << 0))
    {
        Ar << TargetActor;
    }

    return bOutSuccess;
}

// ==================== 게임플레이 이벤트 페이로드 NetSerialize 구현 ====================

/**
 * @brief FPayload_GameplayEventData의 NetSerialize 구현
 * @details 모든 게임플레이 이벤트의 기본 데이터를 직렬화
 */
bool FPayload_GameplayEventData::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

    uint8 RepBits = 0;
    if (Ar.IsSaving())
    {
        if (Instigator.IsValid())
        {
            RepBits |= 1 << 0;
        }
        if (Target.IsValid())
        {
            RepBits |= 1 << 1;
        }
        if (EventLocation.IsNearlyZero())
        {
            RepBits |= 1 << 2;
        }
        if (EventDirection.IsNearlyZero())
        {
            RepBits |= 1 << 3;
        }
        if (EventTime != 0.f)
        {
            RepBits |= 1 << 4;
        }
        if (EventLevel >= 0)
        {
            RepBits |= 1 << 5;
        }
    }

    Ar.SerializeBits(&RepBits, 5);

    if (RepBits & (1 << 0))
    {
        Ar << Instigator;
    }
    if (RepBits & (1 << 1))
    {
        Ar << Target;
    }
    if (RepBits & (1 << 2))
    {
        Ar << EventLocation;
    }
    if (RepBits & (1 << 3))
    {
        Ar << EventDirection;
    }
    if (RepBits & (1 << 3))
    {
        Ar << EventTime;
    }
    if (RepBits & (1 << 3))
    {
        Ar << EventLevel;
    }

    return bOutSuccess;
}

/**
 * @brief FPayload_CrowdControlEvent의 NetSerialize 구현
 */
bool FPayload_CrowdControlEvent::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;

    // 기본 게임플레이 이벤트 데이터 직렬화
    if (!Super::NetSerialize(Ar, Map, bOutSuccess))
    {
        return false;
    }

    Type.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    Ar << Duration;
    Ar << Magnitude;

    return bOutSuccess;
}

/**
 * @brief FPayload_DamageEvent의 NetSerialize 구현
 * @details 안전한 타겟 참조를 위해 액터 포인터 대신 액터 이름과 위치 정보를 사용
 */
bool FPayload_DamageEvent::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;

    // 기본 게임플레이 이벤트 데이터 직렬화
    if (!Super::NetSerialize(Ar, Map, bOutSuccess))
    {
        return false;
    }

    // 데미지 관련 데이터 직렬화
    Ar << Amount;
    Ar << bIsCritical;

	DamageTypes.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
	}
    
    Ar << SourceActionTag;
    Ar << SourceActionTypeTag;

    return bOutSuccess;
}

/**
 * @brief FPayload_HealingEvent의 NetSerialize 구현
 */
bool FPayload_HealingEvent::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;

    // 기본 게임플레이 이벤트 데이터 직렬화
    if (!Super::NetSerialize(Ar, Map, bOutSuccess))
    {
        return false;
    }

    // Healing 관련 데이터 직렬화
    Ar << Amount;
    HealingTypes.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    SourceActionTag.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    return bOutSuccess;
}

/**
 * @brief FPayload_StatusEffectEvent의 NetSerialize 구현
 */
bool FPayload_StatusEffectEvent::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;

    // 기본 게임플레이 이벤트 데이터 직렬화
    if (!Super::NetSerialize(Ar, Map, bOutSuccess))
    {
        return false;
    }

    // StatusEffect 관련 데이터 직렬화
    EffectType.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    Ar << Duration;
    Ar << StackCount;
    Ar << Magnitude;

    SourceActionTag.NetSerialize(Ar, Map, bOutSuccess);
    if (!bOutSuccess)
    {
        return false;
    }

    return bOutSuccess;
}

// ==================== 컨텍스트 페이로드 NetSerialize 구현 ====================

bool FContextPayload::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;

    // 키 배열 직렬화/역직렬화
    Ar << DataKeys;
    if (!bOutSuccess)
    {
        return false;
    }

    // 데이터 배열 크기 직렬화/역직렬화
    int32 ArraySize = DataArray.Num();
    Ar << ArraySize;

    if (Ar.IsLoading())
    {
        DataArray.Empty();
        DataArray.SetNum(ArraySize);
    }

    // 각 FInstancedStruct의 내장 NetSerialize 사용
    for (int32 i = 0; i < ArraySize; ++i)
    {
        if (!DataArray[i].NetSerialize(Ar, Map, bOutSuccess))
        {
            return false;
        }
    }

    return bOutSuccess;
}