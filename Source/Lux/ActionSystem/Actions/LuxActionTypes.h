// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/LuxActionSystemTypes.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h" 
#include "LuxActionTypes.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLuxTaskEvent, const FGameplayTag&, EventTag, const FContextPayload&, Payload);


class ULuxAction;
class ULuxActionTask;


UENUM(BlueprintType)
enum class ELuxActionInstancingPolicy : uint8
{
	/** 인스턴스를 생성하지 않습니다. 항상 클래스 기본 객체(CDO)를 사용합니다. */
	NonInstanced,

	/** 액터에게 부여(Grant)될 때, 액터당 하나의 고유한 인스턴스를 생성합니다. */
	InstancedPerActor,

	/** 액션이 활성화될 때마다 매번 새로운 인스턴스를 생성합니다. */
	InstancedPerExecution
};

UENUM()
enum class EActionReplicatedEvent : uint8
{
	None,
	InputPressed,
	InputReleased,
};

/**
 * 어빌리티의 활성화 방식을 정의합니다.
 */
UENUM(BlueprintType)
enum class ELuxActionActivationPolicy : uint8
{
	// 입력(Input)이 트리거될 때 어빌리티를 활성화 시도합니다.
	OnInputTriggered,

	// 입력이 활성 상태인 동안 어빌리티를 지속적으로 활성화 시도합니다.
	WhileInputActive,

	// 아바타(Avatar)가 할당될 때 어빌리티를 활성화 시도합니다.
	OnSpawn,

	// 어빌리티가 액션 시스템에 부여될 때 어빌리티를 활성화 시도합니다. 
	OnGrant,

	// 어빌리티가 액션 시스템에 부여될 때 활성화되고, 한 번 실행 후 자동으로 제거됩니다.
	OnGrantAndRemove,
};



// 액션을 참조하고 식별하기 위한 핸들 구조체
USTRUCT(BlueprintType)
struct FLuxActionSpecHandle
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 Handle = -1;

public:
	FLuxActionSpecHandle()
		: Handle(-1)
	{
	}

	bool IsValid() const
	{
		return Handle != -1;
	}

	void GenerateNewHandle();
	bool operator==(const FLuxActionSpecHandle& Other) const { return Handle == Other.Handle; }
	friend uint32 GetTypeHash(const FLuxActionSpecHandle& InHandle) { return GetTypeHash(InHandle.Handle); }
	
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};



USTRUCT(BlueprintType)
struct FLuxActionSpec : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FLuxActionSpec(const FLuxActionSpec&) = default;
	FLuxActionSpec(FLuxActionSpec&&) = default;
	FLuxActionSpec& operator=(const FLuxActionSpec&) = default;
	FLuxActionSpec& operator=(FLuxActionSpec&&) = default;
	~FLuxActionSpec() = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

public:
	FLuxActionSpec();
	FLuxActionSpec(TSubclassOf<ULuxAction> ActionClass, const FGameplayTag& InInputTag, int32 InLevel = 1);
	FLuxActionSpec(ULuxAction* InActionCDO, const FGameplayTag& InInputTag, int32 InLevel = 1);

	/** InputTag를 기반으로 Spec에 대한 고유한 Cooldown 태그를 생성하여 반환합니다. */
	FGameplayTag GetCooldownTag() const;

	/** InputTag를 기반으로 Spec에 대한 고유한 Stack(충전) 태그를 생성하여 반환합니다. */
	FGameplayTag GetStackTag() const;

	/** InputTag를 기반으로 Spec에 대한 고유한 MultiCast(연속 사용 횟수) 태그를 생성하여 반환합니다. */
	FGameplayTag GetMultiCastCountTag() const;

	bool IsActive() const { return Action && ActivationCount > 0; };

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	void PreReplicatedRemove(const struct FActionSpecContainer& InArraySerializer);
	void PostReplicatedAdd(const struct FActionSpecContainer& InArraySerializer);
	void PostReplicatedChange(const struct FActionSpecContainer& InArraySerializer);

public:
	/** 이 Spec이 나타내는 액션 CDO */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ActionSpec")
	TObjectPtr<ULuxAction> Action;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ActionSpec")
	FLuxActionSpecHandle Handle;

	/** 이 액션을 발동시키는 '주' 입력 태그입니다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ActionSpec")
	FGameplayTag InputTag;

	/** 이 액션을 고유하게 식별하는 태그 (ULuxAction::ActionIdentifierTag 복제본) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ActionSpec")
	FGameplayTag ActionIdentifierTag;

	/** 이 액션이 반응할 수 있는 모든 동적인 태그 목록. (주 입력 태그 포함) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ActionSpec")
	FGameplayTagContainer DynamicTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ActionSpec")
	int32 Level = 1;

	/** Action 이 현재 입력되고 있는 지 */ 
	UPROPERTY()
	bool InputPressed = false;

	/** Spec의 활성화 횟수 */
	UPROPERTY()
	int32 ActivationCount = 0;

	/** 이 Spec이 마지막으로 활성화된 월드 시간입니다. (콤보 타이밍 계산 등에 사용) */
	UPROPERTY()
	float LastExecutionTime = 0.f;
};

/**
 * @struct FActiveLuxActionHandle
 * @brief 현재 활성화되어 실행 중인 단일 액션 인스턴스를 고유하게 식별하는 핸들입니다.
 * 이 핸들은 가볍고 복제하기 안전하며, 네트워크를 통해 전송될 수 있습니다.
 */
USTRUCT(BlueprintType)
struct FActiveLuxActionHandle
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Handle;

	static int32 Counter;

public:
	FActiveLuxActionHandle();

	/** 이 핸들이 유효한 활성 액션을 가리키는지 확인합니다. */
	bool IsValid() const;

	/** 전역적으로 고유한 새 핸들을 생성합니다. */
	static FActiveLuxActionHandle GenerateNewHandle();

	/** 두 핸들이 동일한지 비교하는 연산자입니다. */
	bool operator==(const FActiveLuxActionHandle& Other) const;

	/** 두 핸들이 다른지 비교하는 연산자입니다. */
	bool operator!=(const FActiveLuxActionHandle& Other) const;

	/** TMap과 같은 해시 기반 컨테이너에서 사용될 해시 값을 생성합니다. */
	friend uint32 GetTypeHash(const FActiveLuxActionHandle& InHandle)
	{
		return GetTypeHash(InHandle.Handle);
	}

	/** 디버깅을 위해 핸들의 ID를 문자열로 변환합니다. */
	FString ToString() const
	{
		return FString::FromInt(Handle);
	}
};

inline int32 FActiveLuxActionHandle::Counter = 0;



/**
 * @struct FActiveLuxAction
 * @brief 현재 활성화되어 실행 중인 단일 액션의 모든 런타임 상태를 저장하는 구조체입니다.
 */
USTRUCT()
struct FActiveLuxAction : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	FActiveLuxAction() = default;
	FActiveLuxAction(const FLuxActionSpec& InSpec, const FLuxPredictionKey& InPredictionKey, const FOwningActorInfo& InActorInfo);

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	// FastArraySerializer에서 사용할 콜백 함수들
	void PreReplicatedRemove(const struct FActiveLuxActionContainer& InArraySerializer);
	void PostReplicatedAdd(const struct FActiveLuxActionContainer& InArraySerializer);
	void PostReplicatedChange(const struct FActiveLuxActionContainer& InArraySerializer);

	/** 동일한지 비교하는 연산자입니다. */
	bool operator==(const FActiveLuxAction& Other) const;

	/** 다른지 비교하는 연산자입니다. */
	bool operator!=(const FActiveLuxAction& Other) const;

public:
	/** 실행될 액션 인스턴스. */
	UPROPERTY()
	TObjectPtr<ULuxAction> Action;

	/** 이 활성화 정보의 고유 핸들 */
	UPROPERTY()
	FActiveLuxActionHandle Handle;

	/** 이 활성화의 기반이 된 원본 Spec */
	UPROPERTY()
	FLuxActionSpec Spec;

	/** 이 활성화와 연결된 예측 키 */
	UPROPERTY()
	FLuxPredictionKey PredictionKey;

	/** 이 활성화가 시작될 때의 ActorInfo */
	UPROPERTY()
	FOwningActorInfo ActorInfo;

	/** 활성화가 시작된 월드 시간입니다. */
	UPROPERTY()
	float StartTime = 0.f;

	/** 이 활성화가 종료되었는지 여부 */
	UPROPERTY()
	bool bIsDone = false;
};


// Spec들의 리스트 컨테이너 (리플리케이션에 사용)
USTRUCT()
struct FActionSpecContainer : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

public:
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FLuxActionSpec, FActionSpecContainer>(Items, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FLuxActionSpec> Items;

	UPROPERTY(Transient)
	TWeakObjectPtr<UActionSystemComponent> OwnerComponent;
};


USTRUCT()
struct FActiveLuxActionContainer : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

public:
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FActiveLuxAction, FActiveLuxActionContainer>(Items, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FActiveLuxAction> Items;

	UPROPERTY(Transient)
	TWeakObjectPtr<UActionSystemComponent> OwnerComponent;
};


template<>
struct TStructOpsTypeTraits<FLuxActionSpec> : public TStructOpsTypeTraitsBase2<FLuxActionSpec>
{
    enum { WithNetSerializer = true };
};

template<>
struct TStructOpsTypeTraits<FLuxActionSpecHandle> : public TStructOpsTypeTraitsBase2<FLuxActionSpecHandle>
{
	enum { WithNetSerializer = true };
};

template<>
struct TStructOpsTypeTraits<FActionSpecContainer> : public TStructOpsTypeTraitsBase2<FActionSpecContainer>
{
	enum { WithNetDeltaSerializer = true };
};

template<>
struct TStructOpsTypeTraits<FActiveLuxActionContainer> : public TStructOpsTypeTraitsBase2<FActiveLuxActionContainer>
{
	enum { WithNetDeltaSerializer = true };
};