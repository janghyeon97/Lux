// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuxAttributeSet.generated.h"

struct FLuxModCallbackData;
struct FLuxEffectSpec;
struct FAttributeModifier;
class UActionSystemComponent;

/**
 * @struct FLuxAttributeData
 * @brief 속성의 실제 숫자 값(BaseValue, CurrentValue)을 저장하는 순수한 데이터 컨테이너입니다.
 *
 * 이 구조체는 자신이 'Health'인지 'Mana'인지와 같은 메타데이터는 알지 못하며, 오직 값의 '실체'만을 담당합니다.
 * 예를 들어 캐릭터의 체력이 100이라면, 이 구조체는 숫자 100을 보관하는 역할을 합니다.
 * 모든 캐릭터 인스턴스는 각 속성에 해당하는 고유한 FLuxAttributeData 인스턴스를 소유하게 됩니다.
 */
USTRUCT(BlueprintType)
struct LUX_API FLuxAttributeData
{
	GENERATED_BODY()

	FLuxAttributeData()
		: BaseValue(0.f)
		, CurrentValue(0.f)
	{
	}

	FLuxAttributeData(float DefaultValue)
		: BaseValue(DefaultValue)
		, CurrentValue(DefaultValue)
	{
	}

	virtual ~FLuxAttributeData()
	{
	}

	/** 일시적인 버프가 모두 포함된 현재 값을 반환합니다. */
	float GetCurrentValue() const;

	/** 현재 값을 변경합니다. */
	virtual void SetCurrentValue(float NewValue);

	/** 영구적인 변화가 반영된 Base 값을 반환합니다. */
	float GetBaseValue() const;

	/** 영구적으로 Base 값을 변경합니다. */
	virtual void SetBaseValue(float NewValue);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Attribute")
	float BaseValue;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute")
	float CurrentValue;
};


/**
 * @struct FLuxAttribute
 * @brief 특정 FLuxAttributeData를 찾아갈 수 있는 '주소' 또는 '식별자' 역할을 하는 구조체입니다.
 *
 * 이 구조체는 실제 능력치 값(예: 100)을 저장하는 것이 아니라, "UHealthSet 클래스 내부의 Health 프로퍼티"와 같이
 * 해당 데이터가 어디에 있는지를 나타내는 경로 정보(TFieldPath)를 저장합니다.
 *
 * 데이터의 '실체'(FLuxAttributeData)와 '식별자'(FLuxAttribute)를 분리함으로써, ULuxEffect와 같은
 * 데이터 애셋에서 "어떤 캐릭터의" 속성이 아닌 "어떤 종류의" 속성을 수정할지 안전하게 지정할 수 있습니다.
 * 이는 데이터 기반 설계를 가능하게 하고, 직렬화 및 네트워킹 시 안정성을 확보하는 핵심적인 설계입니다.
 */
USTRUCT(BlueprintType)
struct FLuxAttribute
{
	GENERATED_BODY()

public:
	FLuxAttribute() = default;

    FLuxAttribute(FProperty* InProperty)
    {
        Attribute = TFieldPath<FProperty>(InProperty);
    }

    /** 이 Attribute가 유효한 프로퍼티를 가리키고 있는지 확인합니다 */
	bool IsValid() const;

    /** TFieldPath에 저장된 경로를 통해 실제 FProperty 포인터를 반환합니다. */
	FProperty* GetProperty() const;

    /** 이 Attribute가 속한 AttributeSet의 UClass를 반환합니다. */
	UClass* GetAttributeSetClass() const;

    /**
     * 주어진 AttributeSet 인스턴스에서 이 Attribute에 해당하는 실제 FLuxAttributeData의 포인터를 가져옵니다.
     * @param AttributeSetInstance 실제 데이터가 담겨있는 AttributeSet 객체
     * @return FLuxAttributeData에 대한 포인터.
     */
	const FLuxAttributeData* GetAttributeData(const ULuxAttributeSet* Src) const;
	const FLuxAttributeData* GetAttributeDataChecked(const ULuxAttributeSet* Src) const;
	FLuxAttributeData* GetAttributeData(ULuxAttributeSet* Src) const;
	FLuxAttributeData* GetAttributeDataChecked(ULuxAttributeSet* Src) const;

    /** 속성의 이름을 FName으로 반환합니다. */
    FName GetName() const
    {
        return IsValid() ? Attribute->GetFName() : NAME_None;
    }

    friend uint32 GetTypeHash(const FLuxAttribute& InAttribute)
    {
        return PointerHash(InAttribute.Attribute.Get());
    }

	bool operator==(const FLuxAttribute& Other) const;
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

protected:
	friend class FLuxAttributeCustomization;

	UPROPERTY(Category = LuxAttribute, EditAnywhere, meta = (AllowPrivateAccess))
	TFieldPath<FProperty> Attribute;
};

template<>
struct TStructOpsTypeTraits<FLuxAttribute> : public TStructOpsTypeTraitsBase2<FLuxAttribute>
{
	enum { WithNetSerializer = true };
};

/**
 * @class ULuxAttributeSet
 * @brief 모든 속성(Attribute)의 기반이 되는 클래스입니다.
 * 이 클래스는 속성 값의 변경 전/후 과정을 제어하는 가상 함수들을 제공하여,
 * 이펙트가 적용되는 전체 데이터 흐름을 관리합니다.
 * 
 * * ## 이펙트 실행 흐름 (Execution Pipeline)
 * * 하나의 이펙트가 적용될 때, 이 클래스 내의 함수들은 아래와 같은 순서로 호출됩니다.
 *
 * 1. [ Modifiers 계산 ]			(ExecutionCalculation이 최종 Modifier를 계산합니다)
 * 2. PreLuxEffectExecute		(이펙트 적용 전 최종 검증)
 * 3. PreAttributeBaseChange	(BaseValue 변경 전, 값 보정)
 * 4. [ 실제 BaseValue 변경 ]
 * 5. PostAttributeBaseChange	(BaseValue 변경 후, 값 보정)
 * 6. PreAttributeChange		(CurrentValue 변경 전, 값 보정)
 * 7. [ 실제 CurrentValue 변경 ]
 * 8. PostAttributeChange		(CurrentValue 변경 후, UI 업데이트 등)
 * 9. PostLuxEffectExecute		(이펙트 적용 후 최종 결과 처리)
 */
UCLASS(Blueprintable)
class LUX_API ULuxAttributeSet : public UObject
{
	GENERATED_BODY()

public:
    ULuxAttributeSet();

    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	AActor* GetOwningActor() const;
	UActionSystemComponent* GetOwningActionSystemComponent() const;
	UActionSystemComponent* GetOwningActionSystemComponentChecked() const;

	/** 이펙트가 속성에 적용되기 직전에 호출됩니다. 전체 이펙트 적용을 막을 수 있습니다. */
	virtual bool PreLuxEffectExecute(FLuxModCallbackData& Data) { return true; }

	/** 이펙트가 속성에 적용된 후에 호출됩니다. */
	virtual void PostLuxEffectExecute(const FLuxModCallbackData& Data) {}

	/**
	 * BaseValue 값이 변경되기 직전에 호출됩니다.
	 * 
	 * @param Attribute  변경될 속성의 데이터
	 * @param NewValue   변경될 값
	 */
	virtual void PreAttributeBaseChange(const FLuxAttribute& Attribute, float& NewValue) {};

	/**
	 * BaseValue 값이 변경된 직후에 호출됩니다.
	 *
	 * @param OldValue  이전 값
	 * @param NewValue  변경된 값
	 */
	virtual void PostAttributeBaseChange(const FLuxAttribute& Attribute, float OldValue, float NewValue) {};

	/**
	 * CurrentValue 값이 변경되기 직전에 호출됩니다.
	 * 
	 * @param Attribute  변경될 속성의 데이터
	 * @param NewValue   변경될 값
	 */
	virtual void PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue) {};

	/**
	 * CurrentValue 값이 변경된 직후에 호출됩니다.
	 * 
	 * @param OldValue  이전 값
	 * @param NewValue  변경된 값
	 */
	virtual void PostAttributeChange(const FLuxAttribute& Attribute, float OldValue, float NewValue) {};
};


/**
 * attribute event를 브로드캐스트하는 델리게이트입니다.
 * @param EffectInstigator    이 이벤트를 시작시킨 액터 (instigating actor)
 * @param EffectCauser        변화를 직접적으로 유발한 액터 (causing actor)
 * @param EffectSpec          이 변화를 일으킨 전체 이펙트 스펙 (effect spec)
 * @param EffectMagnitude     클램핑(clamping) 전의 순수한 변화 크기 (raw magnitude)
 * @param OldValue            적용 전의 속성 값
 * @param NewValue            적용 후의 속성 값
 */
DECLARE_MULTICAST_DELEGATE_SixParams(FLuxAttributeEvent,
	AActor* /*EffectInstigator*/,
	AActor* /*EffectCauser*/,
	const FLuxEffectSpec* /*EffectSpec*/,
	float /*EffectMagnitude*/,
	float /*OldValue*/,
	float /*NewValue*/);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLuxAttributeChanged,
	float, OldValue,
	float, NewValue);

/*
 #define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	LUXATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	LUXPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	LUXPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	LUXPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
 
 ATTRIBUTE_ACCESSORS(UMyHealthSet, Health)
 */

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	LUXATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	LUXPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	LUXPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	LUXPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


#define LUXATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	static FLuxAttribute Get##PropertyName##Attribute() \
	{ \
		static FProperty* Prop = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
		return Prop; \
	}

#define LUXPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	FORCEINLINE float Get##PropertyName() const \
	{ \
		return PropertyName.GetCurrentValue(); \
	}

#define LUXPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	FORCEINLINE void Set##PropertyName(float NewVal) \
	{ \
		UActionSystemComponent* ASC = GetOwningActionSystemComponent(); \
		if (ensure(ASC)) \
		{ \
			ASC->SetNumericAttributeBase(Get##PropertyName##Attribute(), NewVal); \
		}; \
	}

#define LUXPLAYATTRIBUTE_VALUE_INITTER(PropertyName) \
	FORCEINLINE void Init##PropertyName(float NewVal) \
	{ \
		PropertyName.SetBaseValue(NewVal); \
		PropertyName.SetCurrentValue(NewVal); \
	}

#define LUXATTRIBUTE_REPNOTIFY(ClassName, PropertyName, OldValue) \
	{ \
		static FProperty* Prop = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
		if (Prop) \
		{ \
			const float OldValue_ = OldValue.GetCurrentValue(); \
			const float NewValue_ = Get##PropertyName(); \
			On##PropertyName##Changed.Broadcast(OldValue_, NewValue_); \
		} \
	}