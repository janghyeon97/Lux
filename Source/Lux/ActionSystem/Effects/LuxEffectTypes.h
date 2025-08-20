// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h" 
#include "ActionSystem/LuxActionSystemTypes.h"
#include "ActionSystem/Actions/LuxActionTypes.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "GameplayTagContainer.h"

#include "LuxEffectTypes.generated.h"

class AActor;
class UActionSystemComponent;
class ULuxEffect;
class ULuxAction;
struct FLuxActionSpecHandle;

#pragma region Define Types
/* ======================================== Effect Management ======================================== */
UENUM(BlueprintType)
enum class ELuxEffectDurationPolicy : uint8
{
    /** 즉시 적용되고 바로 소멸합니다. BaseValue를 변경합니다. */
    Instant,

    /** 제거되기 전까지 영구적으로 지속됩니다. CurrentValue에 영향을 줍니다. */
    Infinite,

    /** 정해진 시간 동안 지속됩니다. CurrentValue에 영향을 줍니다. */
    HasDuration
};


UENUM(BlueprintType)
enum class EValueCalculationType : uint8
{
    /* 고정값을 사용합니다. */
    Static,

    /* 런타임에 호출자가 값을 결정합니다. */
    SetByCaller
};


USTRUCT(BlueprintType)
struct FLuxScalableFloat
{
    GENERATED_BODY()

public:
    /** 이 값의 수치를 어떻게 계산할지 결정합니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ScalableFloat")
    EValueCalculationType CalculationType = EValueCalculationType::Static;

    /** CalculationType이 Static일 때 사용할 고정 값입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ScalableFloat", meta = (EditCondition = "CalculationType == EValueCalculationType::Static", EditConditionHides))
    float StaticValue = 0.0f;

    /** CalculationType이 SetByCaller일 때, 이 값을 식별하기 위한 태그입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ScalableFloat", meta = (EditCondition = "CalculationType == EValueCalculationType::SetByCaller", EditConditionHides))
    FGameplayTag CallerTag;
};


/**
 * @enum EModifierOperation
 * @brief 속성 변경자가 어떤 연산을 수행할지 정의하는 열거형입니다.
 */
UENUM(BlueprintType)
enum class EModifierOperation : uint8
{
    Add,
    Multiply,
    Override
};

/**
 * @enum EEffectStackingType
 * @brief 동일한 이펙트가 여러 번 적용될 때 어떻게 처리할지 정의하는 열거형입니다.
 */
UENUM(BlueprintType)
enum class EEffectStackingType : uint8
{
    /* 스택을 허용하지 않으며 항상 별개의 인스턴스로 적용됩니다. */
    None,

    /* 스택 카운트를 누적하지만 지속시간은 초기화하지 않습니다. (예: 출혈 5중첩) */
    Aggregate,

    /* 스택 카운트를 누적하고 지속시간을 새로 적용된 이펙트의 시간으로 갱신합니다. */
    Replace
};

/**
 * @struct FAttributeModifier
 * @brief 단일 속성을 어떻게 변경할지에 대한 모든 정보를 담는 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FAttributeModifier
{
    GENERATED_BODY()

public:
    /* 변경할 속성의 식별자 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLuxAttribute Attribute;

    /* 적용할 연산 방식 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EModifierOperation Operation = EModifierOperation::Add;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLuxScalableFloat Magnitude;
};
#pragma endregion


#pragma region Define Handles 
/**
 * @struct FLuxEffectContextHandle
 * @brief FLuxEffectContext 에 대한 핸들입니다.
 * 컨텍스트 구조체 자체는 크기가 클 수 있으므로, 네트워크로 전송하거나 복사할 때
 * 이 가벼운 핸들을 사용하여 데이터 중복을 피하고 효율성을 높입니다.
 */
USTRUCT(BlueprintType)
struct FLuxEffectContextHandle
{
    GENERATED_BODY()

    // TODO: 실제 시스템에서는 UActionSystemGlobals와 같은 중앙 관리자에서
    // 컨텍스트 객체 배열을 관리하고, 이 핸들은 해당 배열의 인덱스나 고유 ID를 가리키게 됩니다.
public:
    FLuxEffectContextHandle() {}
    virtual ~FLuxEffectContextHandle() {}

    explicit FLuxEffectContextHandle(FLuxEffectContext* ContextPtr)
    {
        Context = TSharedPtr<FLuxEffectContext>(ContextPtr);
    }

    void operator=(FLuxEffectContext* ContextPtr)
    {
        Context = TSharedPtr<FLuxEffectContext>(ContextPtr);
    }

    bool operator==(const FLuxEffectContextHandle& Other) const;

public:
    /** 핸들이 유효한 컨텍스트를 가리키고 있는지 확인합니다. */
    bool IsValid() const { return Context.IsValid(); }

    /** 새로운 컨텍스트 데이터를 생성하고 핸들이 가리키도록 합니다. */
    void NewContext() { Context = MakeShared<FLuxEffectContext>(); }

    /** FLuxEffectContext 포인터를 반환합니다. */
    FLuxEffectContext* Get() { return IsValid() ? Context.Get() : nullptr; }
    const FLuxEffectContext* Get() const { return IsValid() ? Context.Get() : nullptr; }

    AActor* GetInstigator() const;
    AActor* GetEffectCauser() const;
    UActionSystemComponent* GetTargetASC() const;
    UActionSystemComponent* GetSourceASC() const;
    FLuxActionSpecHandle GetSourceAction() const;

    void SetInstigator(AActor* InActor);
    void SetEffectCauser(AActor* InActor);
    void SetTargetASC(UActionSystemComponent* InASC);
    void SetSourceASC(UActionSystemComponent* InASC);
    void SetSourceAction(const FLuxActionSpecHandle& InHandle);

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

private:
    TSharedPtr<FLuxEffectContext> Context;
};

/**
 * @struct FLuxEffectSpecHandle
 * @brief FLuxEffectSpec에 대한 핸들입니다.
 * 이 핸들을 통해 실제 Spec 데이터에 안전하고 편리하게 접근할 수 있습니다.
 */
USTRUCT(BlueprintType)
struct FLuxEffectSpecHandle
{
    GENERATED_BODY()

public:
    FLuxEffectSpecHandle() = default;

    FLuxEffectSpecHandle(TSharedPtr<FLuxEffectSpec> InSpec)
        : Spec(InSpec)
    {
    }

    FLuxEffectSpecHandle(FLuxEffectSpec* SpecPtr)
        : Spec(SpecPtr)
    {
    }

public:
    /** 핸들이 유효한 Spec을 가리키고 있는지 확인합니다. */
    bool IsValid() const
    {
        return Spec.IsValid();
    }

    /** 내부의 Spec 데이터 포인터를 직접 가져옵니다. */
    FLuxEffectSpec* Get() const
    {
        return Spec.Get();
    }

    bool operator==(const FLuxEffectSpecHandle& Other) const
    {
        return Spec == Other.Spec;
    }

    bool operator!=(const FLuxEffectSpecHandle& Other) const
    {
        return Spec != Other.Spec;
    }

private:
    /** 실제 Spec 데이터에 대한 공유 포인터입니다. */
    TSharedPtr<FLuxEffectSpec> Spec;
};

/**
 * @struct FActiveLuxEffectHandle
 * @brief 활성화된(지속 중인) 이펙트 인스턴스를 고유하게 식별하는 핸들입니다.
 */
USTRUCT(BlueprintType)
struct FActiveLuxEffectHandle
{
    GENERATED_BODY()

public:
    FActiveLuxEffectHandle() : Handle(-1) {}

    static FActiveLuxEffectHandle GenerateNewHandle();

    bool IsValid() const { return Handle != -1; }

    bool operator==(const FActiveLuxEffectHandle& Other) const;
    bool operator!=(const FActiveLuxEffectHandle& Other) const;

    friend uint32 GetTypeHash(const FActiveLuxEffectHandle& InHandle)
    {
        return GetTypeHash(InHandle.Handle);
    }

    FString ToString() const
    {
        return FString::FromInt(Handle);
    }

public:
    UPROPERTY()
    int32 Handle = -1; // 고유 ID

    static int32 Counter;
};
#pragma endregion


#pragma region Lux Effect
/**
 * @struct FLuxEffectContext
 * @brief 이펙트가 생성되고 적용되는 순간의 모든 '상황 정보(Context)'를 담는 구조체입니다. ( 사건 경위서와 유사한 역할 )
 * 이 정보는 데미지 계산 등 복잡한 로직에 사용됩니다.
 */
USTRUCT(BlueprintType)
struct FLuxEffectContext
{
    GENERATED_BODY()

public:
    AActor* GetInstigator() const;
    AActor* GetEffectCauser() const;
    UActionSystemComponent* GetTargetASC() const;
    UActionSystemComponent* GetSourceASC() const;
    FLuxActionSpecHandle GetSourceAction() const;

    void SetInstigator(AActor* InActor);
    void SetEffectCauser(AActor* InActor);
    void SetTargetASC(UActionSystemComponent* InASC);
    void SetSourceASC(UActionSystemComponent* InASC);
    void SetSourceAction(const FLuxActionSpecHandle& InHandle);

    bool operator==(const FLuxEffectContext& Other) const;

    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

public:
    /** 이 효과를 최초로 유발시킨 액터 (예: 플레이어 캐릭터) */
    UPROPERTY()
    TWeakObjectPtr<AActor> Instigator;

    /** 이 효과를 직접적으로 발생시킨 객체 (예: 캐릭터가 쏜 발사체) */
    UPROPERTY()
    TWeakObjectPtr<AActor> EffectCauser;

    /** 효과를 적용받는 대상의 ASC */
    UPROPERTY()
    TWeakObjectPtr<UActionSystemComponent> TargetASC;

    /** 효과를 발생시킨 소스의 ASC */
    UPROPERTY()
    TWeakObjectPtr<UActionSystemComponent> SourceASC;

    /** 이 효과를 발생시킨 액션의 Spec 핸들 */
    UPROPERTY()
    FLuxActionSpecHandle SourceActionHandle;
};




/**
 * @struct FLuxEffectSpec
 * @brief 런타임에 ULuxEffect를 기반으로 생성되는 이펙트 인스턴스입니다.
 * @brief "사건 경위서(Context)"와 "레시피 원본(ULuxEffect)"을 가지고 실제로 실행할 최종 "주문서(Spec)"입니다.
 */
USTRUCT(BlueprintType)
struct FLuxEffectSpec
{
    GENERATED_BODY()

public:
    FLuxEffectSpec();
    FLuxEffectSpec(const ULuxEffect* InEffectTemplate, float InLevel, const FLuxEffectContextHandle& InContextHandle);
    FLuxEffectSpec(const FLuxEffectSpec& Other);

    /** 이 Spec이 유효한지 간단히 확인하는 헬퍼 함수 */
    bool IsValid() const;

    /** SetByCaller로 지정된 Modifier의 Magnitude 값을 런타임에 설정합니다. */
    void SetByCallerMagnitude(const FGameplayTag& Tag, float Value);

    /** SetByCaller로 지정된 Modifier의 Magnitude 값을 가져옵니다. */
    float GetByCallerMagnitude(FGameplayTag Tag, bool bWarnIfNotFound = true, float DefaultValue = 0.f) const;

public:
    /**
     * 이 Spec의 원본이 된 Effect 템플릿의 기본 객체(CDO)에 대한 포인터입니다.
     * 이 포인터를 통해 Duration, Period, StackingType 등 기본 정보를 가져옵니다.
     */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec")
    TWeakObjectPtr<const ULuxEffect> EffectTemplate;

    /**
     * 이 Spec이 생성될 때의 컨텍스트 정보에 대한 핸들입니다.
     * 시전자, 대상, 발생 위치 등의 정보를 담고 있습니다.
     */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec")
    FLuxEffectContextHandle ContextHandle;

    /**
     * 원본 템플릿의 Modifier와 컨텍스트(레벨, 시전자 스탯 등)를
     * 종합하여 최종 계산된 Modifier 목록입니다.
     * 이 목록의 값들이 대상의 속성에 실제로 적용됩니다.
     */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec")
    TArray<FAttributeModifier> CalculatedModifiers;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec")
	TWeakObjectPtr<AActor> SourceObject;

    /** Effect 종류를 정의하는 태그입니다. (예: "State.CrowdControl.Root") */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec|Tags")
    FGameplayTagContainer DynamicEffectTags;

    /** 이 효과가 적용되는 동안 대상에게 부여할 태그들입니다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec|Tags")
    FGameplayTagContainer DynamicGrantedTags;

    /** 이 효과가 적용되기 위해, 대상이 반드시 가지고 있어야 하는 태그들입니다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec|Tags")
    FGameplayTagContainer ApplicationRequiredTags;

    /** 이 효과가 적용되는 것을 막는 태그들입니다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec|Tags")
    FGameplayTagContainer ApplicationBlockedTags;

    /** 이 효과가 적용될 때, 대상에게서 제거할 다른 효과들을 태그로 지정합니다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec|Tags")
    FGameplayTagContainer RemoveEffectsWithTags;

    /** 이 Spec이 생성될 때의 레벨입니다.*/
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec")
    float Level = 1.0f;

    /** 효과의 주기 (초). 0 이하면 한 번만 적용됩니다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EffectSpec")
    float CalculatedPeriod = 0.0f;

    /** 효과의 지속 시간 (초). 0 이하는 즉시 또는 무한을 의미. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Effect|Runtime")
    float CalculatedDuration = 0.0f;
};

inline int32 FActiveLuxEffectHandle::Counter = 0;


/**
 * @struct FActiveLuxEffect
 * @brief 현재 대상에게 적용되어 활성화된(지속 중인) 이펙트의 런타임 정보입니다.
 */
USTRUCT()
struct FActiveLuxEffect : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:
    FActiveLuxEffect() = default;
    FActiveLuxEffect(const FLuxEffectSpec& InSpec);

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

public:
    /** 이 활성 이펙트의 고유 핸들 */
    UPROPERTY()
    FActiveLuxEffectHandle Handle;

    /** 이 활성 이펙트의 기반이 된 Spec 정보 */
    UPROPERTY()
    FLuxEffectSpec Spec;

    /** 이 효과가 대상에게 처음 적용된 시간 (게임 시간 기준) */
    UPROPERTY()
    float StartTime = 0.f;

    UPROPERTY()
    float EndTime = 0.f;

    /** 현재 Stack 수 */
    UPROPERTY()
    int32 CurrentStacks = 0;
};


USTRUCT()
struct FActiveLuxEffectsContainer : public FFastArraySerializer
{
    GENERATED_USTRUCT_BODY()

public:
    FActiveLuxEffectsContainer() {}

    void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FActiveLuxEffect, FActiveLuxEffectsContainer>(Items, DeltaParms, *this);
    }

public:
    UPROPERTY()
    TArray<FActiveLuxEffect> Items;

    UPROPERTY(Transient)
    TWeakObjectPtr<UActionSystemComponent> OwnerComponent;
};
#pragma endregion

struct FLuxModCallbackData
{
    FLuxModCallbackData(const FLuxEffectSpec& InSpec, const FLuxAttribute& InAttribute, const FLuxAttributeData& InAttributeData, float InEvaluatedMagnitude, UActionSystemComponent& InTarget)
        : Spec(InSpec), Attribute(InAttribute), AttributeData(InAttributeData), EvaluatedMagnitude(InEvaluatedMagnitude), TargetASC(InTarget)
    {

    }

    /** 이 변경을 유발한 전체 이펙트 스펙입니다. (읽기 전용) */
    const FLuxEffectSpec& Spec;

    /** 현재 적용되고 있는 Attribute 정보입니다. (읽기 전용) */
    const FLuxAttribute& Attribute;

    /** 현재 적용되고 있는 단일 AttributeData 정보입니다. (읽기 전용) */
    const FLuxAttributeData& AttributeData;

    /** 모든 계산과 보정(Clamp)이 끝난 후의 최종 적용 수치입니다. */
    float EvaluatedMagnitude;

    /** 이 변경의 대상이 된 ActionSystemComponent입니다. */
    UActionSystemComponent& TargetASC;
};





#pragma region TStructOpsTypeTraits Settings
/* ======================================== Effect Management ======================================== */
template<>
struct TStructOpsTypeTraits<struct FLuxEffectContextHandle> : public TStructOpsTypeTraitsBase2<FLuxEffectContextHandle>
{
    enum { WithNetSerializer = true };
};

//template<>
//struct TStructOpsTypeTraits<struct FActiveLuxEffectHandle> : public TStructOpsTypeTraitsBase2<FActiveLuxEffectHandle>
//{
//    enum { WithNetSerializer = true };
//};

template<>
struct TStructOpsTypeTraits<struct FActiveLuxEffect> : public TStructOpsTypeTraitsBase2<FActiveLuxEffect>
{
    enum { WithNetSerializer = true };
};

template<>
struct TStructOpsTypeTraits<FActiveLuxEffectsContainer> : public TStructOpsTypeTraitsBase2<FActiveLuxEffectsContainer>
{
    enum { WithNetDeltaSerializer = true };
};
#pragma endregion