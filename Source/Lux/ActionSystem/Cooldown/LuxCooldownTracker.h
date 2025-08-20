#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "LuxCooldownTracker.generated.h"

class UActionSystemComponent;
struct FActiveLuxEffect;

/** 단일 쿨다운 항목 */
USTRUCT()
struct FCooldownEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

public:
	/** 쿨다운을 식별하는 태그 (예: Action.Cooldown.Primary) */
	UPROPERTY()
	FGameplayTag CooldownTag;

	/** 시작 시각(서버 월드 시간) */
	UPROPERTY()
	float StartTime = 0.f;

	/** 종료 시각(서버 월드 시간) */
	UPROPERTY()
	float EndTime = 0.f;

	/** 총 지속 시간(초) */
	UPROPERTY()
	float Duration = 0.f;
};

/** 활성 쿨다운 컨테이너 (리플리케이션용) */
USTRUCT()
struct FCooldownContainer : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FCooldownEntry> Items;

    // 소유 트래커 포인터 (클라이언트 복제 콜백에서 UI 알림을 위해 사용)
    UPROPERTY(Transient)
    class ULuxCooldownTracker* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FCooldownEntry, FCooldownContainer>(Items, DeltaParms, *this);
	}

    // FastArray 복제 콜백들
    void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits<FCooldownContainer> : public TStructOpsTypeTraitsBase2<FCooldownContainer>
{
	enum { WithNetDeltaSerializer = true };
};


/** 쿨다운을 전담 관리하는 트래커 (Replicated Subobject 권장) */
UCLASS(BlueprintType)
class LUX_API ULuxCooldownTracker : public UObject
{
	GENERATED_BODY()

	friend class UActionSystemComponent;
	friend struct FCooldownContainer;

public:
	ULuxCooldownTracker();

public:
	/** 네트워크 복제 지원 여부를 반환합니다. 액션은 항상 네트워크 복제를 지원합니다. */
	FORCEINLINE bool IsSupportedForNetworking() const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** 네트워크로부터 데이터를 받은 후 호출됩니다. (클라이언트에서 Owner 설정용) */
	virtual void PostNetReceive() override;

	/** 서브오브젝트 복제를 처리합니다. 액션의 모든 서브오브젝트를 복제 대상으로 설정합니다. */
	FORCEINLINE virtual bool ReplicateSubobjects(class AActorChannel* Channel, class FOutBunch* Bunch, struct FReplicationFlags* RepFlags)
	{
		return true;
	}

public:
	/** 소유 ASC를 설정합니다. (서브오브젝트 초기화 시 호출) */
	void Initialize(UActionSystemComponent* InOwnerASC);

	/** 서버 전용: 쿨다운 시작/갱신 */
    void StartCooldown(const FGameplayTag& CooldownTag, float Duration);

	/** 서버 전용: 쿨다운 중지 */
	void StopCooldown(const FGameplayTag& CooldownTag);

	/** 이펙트 훅: 쿨다운 이펙트가 추가될 때 호출 */
    void OnCooldownEffectAdded(const FActiveLuxEffect& Effect);

	/** 이펙트 훅: 쿨다운 이펙트가 제거될 때 호출 */
	void OnCooldownEffectRemoved(const FActiveLuxEffect& Effect);

	/** 조회: 남은 시간(초) */
	UFUNCTION(BlueprintPure, Category = "Lux|Cooldown")
    float GetTimeRemaining(const FGameplayTag& CooldownTag) const;

	/** 조회: 총 지속 시간(초) */
	UFUNCTION(BlueprintPure, Category = "Lux|Cooldown")
	float GetDuration(const FGameplayTag& CooldownTag) const;

	/** 서버 전용: 남은 시간을 초 단위로 줄입니다. */
    void ReduceCooldown(const FGameplayTag& CooldownTag, float Seconds);

	/** 서버 전용: 남은 시간을 퍼센트(0~1)로 줄입니다. */
    void ReduceCooldownByPercent(const FGameplayTag& CooldownTag, float Percent);

public:
    // UI 갱신 델리게이트 (클라이언트에서만 브로드캐스트)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCooldownAdded, const FGameplayTag&, CooldownTag, float, Duration);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCooldownChanged, const FGameplayTag&, CooldownTag, float, TimeRemaining, float, Duration);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCooldownRemoved, const FGameplayTag&, CooldownTag);

    UPROPERTY(BlueprintAssignable, Category = "Lux|Cooldown")
    FOnCooldownAdded OnCooldownAdded;

    UPROPERTY(BlueprintAssignable, Category = "Lux|Cooldown")
    FOnCooldownChanged OnCooldownChanged;

    UPROPERTY(BlueprintAssignable, Category = "Lux|Cooldown")
    FOnCooldownRemoved OnCooldownRemoved;

protected:
	/** 소유 ASC (ReplicateSubobjects 등록은 ASC 쪽에서 수행) */
	UPROPERTY(Transient)
	TWeakObjectPtr<UActionSystemComponent> OwnerASC;

	/** 활성 쿨다운 목록 (FastArray 복제) */
	UPROPERTY(Replicated)
	FCooldownContainer Cooldowns;

private:
	int32 FindCooldownIndex(const FGameplayTag& Tag) const;

    // FastArray 콜백에서 호출되어 델리게이트 브로드캐스트를 수행합니다.
    void HandleEntryAdded(const FCooldownEntry& Entry);
    void HandleEntryChanged(const FCooldownEntry& Entry);
    void HandleEntryRemoved(const FGameplayTag& RemovedTag);
};


