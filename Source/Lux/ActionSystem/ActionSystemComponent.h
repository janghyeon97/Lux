// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actions/LuxAction.h"
#include "Actions/LuxActionTypes.h"
#include "Effects/LuxEffectTypes.h"
#include "LuxActionSystemTypes.h"
#include "NativeGameplayTags.h"
#include "GameplayTagContainer.h"
#include "System/GameplayTagStack.h"
#include "Cues/LuxCueNotify.h"

#include "Components/ActorComponent.h"
#include "ActionSystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionActivated, ULuxAction*, ActivatedAction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionFailed, ULuxAction*, FailedAction, const FGameplayTagContainer&, FailureTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionEnded, ULuxAction*, EndedAction, bool, bWasCancelled);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionInputTagPressed, const FGameplayTag&, InputTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionInputTagReleased, const FGameplayTag&, InputTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGameplayTagStackChanged, const FGameplayTag&, Tag, int32, OldCount, int32, NewCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameplayEvent, const FGameplayTag&, EventTag, const FContextPayload&, Payload);

// 네이티브 이펙트 적용/제거 알림 델리게이트 (블루프린트 비노출)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectAppliedNative, const FActiveLuxEffect&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectRemovedNative, const FActiveLuxEffect&);


class ULuxActionTask;
class ULuxAttributeSet;
class APlayerController;
class ULuxActionTagRelationshipMapping;
class ULuxCooldownTracker;

struct FAttributeModifier;





UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LUX_API UActionSystemComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ULuxAction;
	friend class ULuxEffect;
	friend class ULuxPawnExtensionComponent;

	friend struct FActiveLuxEffectsContainer;

public:
	UActionSystemComponent();

	bool bCharacterActionsGiven = false;
	bool bStartupEffectsApplied = false;

#pragma region Component Lifecycle & ActorInfo
	/* ======================================== Component Lifecycle & ActorInfo ======================================== */
public:
	/** 소유자(Owner)와 아바타(Avatar) 액터를 설정하여 컴포넌트를 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|ActorInfo")
	void InitActorInfo(AActor* NewOwnerActor, AActor* NewAvatarActor);

	/** 이 컴포넌트의 모든 상태(액터 정보, 속성, 액션, 활성 이펙트 등)를 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|ActorInfo")
	void ClearActorInfo();

	/** 소유자(Owner) 액터를 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|ActorInfo")
	AActor* GetOwnerActor() const { return OwnerActor.IsValid() ? OwnerActor.Get() : nullptr; }

	/** 아바타(Avatar) 액터를 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|ActorInfo")
	AActor* GetAvatarActor() const { return AvatarActor.IsValid() ? AvatarActor.Get() : nullptr; }

	/** 소유자(Owner) 액터를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|ActorInfo")
	void SetOwnerActor(AActor* NewOwnerActor);

	/** 아바타(Avatar) 액터를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|ActorInfo")
	void SetAvatarActor(AActor* NewAvatarActor);

protected:
	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of UActorComponent interface
#pragma endregion


#pragma region Action Management
	// ======================================== Action Granting & Removal (Authority-Only) ========================================

public:
	/** 지정된 클래스의 액션을 부여합니다. (블루프린트용) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "LuxActionSystem|Actions", meta = (DisplayName = "Grant Action", ScriptName = "GrantAction"))
	FLuxActionSpecHandle K2_GrantAction(TSubclassOf<ULuxAction> ActionClass, FGameplayTag& InputTag, int Level = 1);

	/** Spec으로 정의된 액션을 부여합니다. (C++용) */
	FLuxActionSpecHandle GrantAction(FLuxActionSpec& InSpec);

	/** 부여된 액션을 핸들을 사용해 제거합니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "LuxActionSystem|Actions")
	void RemoveAction(FLuxActionSpecHandle Handle);

	/** 부여된 모든 액션을 제거합니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "LuxActionSystem|Actions")
	void RemoveAllActions();

	// ======================================== Action Activation & Execution ========================================

public:
	/** 액션 활성화를 시도합니다. (클라이언트 진입점) */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Actions")
	void TryExecuteAction(const FLuxActionSpecHandle Handle);

	/** 서버에 액션 실행을 요청합니다. (클라이언트 -> 서버) */
	UFUNCTION(Server, Reliable)
	void Server_TryExecuteAction(const FLuxActionSpecHandle Handle, FLuxPredictionKey PredictionKey);

	/** 서버가 클라이언트의 예측 실행을 확정/거절합니다. (서버 -> 클라이언트) */
	UFUNCTION(Client, Reliable)
	void Client_ConfirmAction(FLuxPredictionKey Key, bool bSuccess);

	/** 클라이언트의 예측 태스크들을 서버의 '진짜' 액션으로 소유권 이전합니다. */
	void ReHomePredictedActionTasks(FActiveLuxAction& AuthoritativeAction);

	/** InstancedPerExecution 정책 액션의 태스크 소유권을 이전합니다. */
	void ReHomeExecutionInstancedAction(FActiveLuxAction& AuthoritativeAction);

	/** InstancedPerActor 정책 액션의 상태를 동기화합니다. */
	void ReHomeActorInstancedAction(FActiveLuxAction& AuthoritativeAction);

	/** 활성화된 액션의 실행을 종료합니다. */
	void EndAction(const FActiveLuxActionHandle& Handle);

	/** 활성화된 액션이 종료되었을 때 호출됩니다. */
	void OnActionEnd(const FActiveLuxActionHandle& Handle, bool bWasCancelled);

	/** 서버가 클라이언트에게 액션이 종료되었음을 알리고 취소 여부를 전달합니다. */
	UFUNCTION(Client, Reliable)
	void Client_NotifyActionEnded(FActiveLuxActionHandle Handle, bool bWasCancelled);

protected:
	// ======================================== Action Activation Checks ========================================

	/** 지정된 액션을 활성화할 수 있는지 모든 조건을 검사합니다. */
	bool CanActivateAction(FLuxActionSpecHandle Handle, FGameplayTagContainer& OutFailureTags) const;

	/** 액션이 재사용 대기시간(Cooldown) 상태인지 확인합니다. */
	virtual bool CheckCooldown(const FLuxActionSpec& Spec, FGameplayTagContainer& OutFailureTags) const;

	/** 지정된 액션의 비용(Cost)과 추가 비용(Additional Costs)을 검사합니다. */
	virtual bool CheckCost(const FLuxActionSpec& Spec, FGameplayTagContainer& OutFailureTags) const;

	/** 활성화를 막는 Blocked 태그를 가지고 있는지 확인합니다. */
	virtual bool HasBlockedTags(const FLuxActionSpec& Spec, const FGameplayTagContainer& ActivationBlockedTags, FGameplayTagContainer& OutFailureTags) const;

	/** 활성화에 필요한 Required 태그를 모두 가지고 있는지 확인합니다. */
	virtual bool HasRequiredTags(const FLuxActionSpec& Spec, const FGameplayTagContainer& ActivationRequiredTags, FGameplayTagContainer& OutFailureTags) const;

	// ======================================== Action Cancellation ========================================

	/** 활성화된 액션을 핸들로 검색하여 취소합니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "LuxActionSystem|Actions")
	void CancelAction(const FActiveLuxActionHandle& Handle);

	/**
	 * 지정된 태그 조건을 만족하는 모든 활성 액션을 취소합니다.
	 * @param WithTags 이 컨테이너의 태그 중 하나라도 가진 액션만 취소됩니다. (nullptr이면 이 조건 무시)
	 * @param WithoutTags 이 컨테이너의 태그 중 하나라도 가진 액션은 취소 대상에서 제외됩니다. (nullptr이면 이 조건 무시)
	 */
	void CancelActions(const FGameplayTagContainer* WithTags, const FGameplayTagContainer* WithoutTags);

	// ======================================== Event-Driven Activation ========================================

	/** 이벤트에 반응하여 액션을 활성화 시도합니다. */
	void TryActivateActionsByEvent(FGameplayTag EventTag, const FContextPayload& Payload);

	/** 이벤트 기반 활성화 시 모든 조건을 검사합니다. Source 태그 검사를 포함합니다. */
	bool CanActivateActionFromEvent(const FLuxActionSpec& Spec, const FContextPayload& Payload, FGameplayTagContainer& OutFailureTags) const;

	// ======================================== Helper & Getter Functions ========================================

public:
	/** 핸들로 부여된 액션의 정보(Spec)를 검색합니다. */
	FLuxActionSpec* FindActionSpecFromHandle(FLuxActionSpecHandle ActionHandle);
	const FLuxActionSpec* FindActionSpecFromHandle(FLuxActionSpecHandle ActionHandle) const;

	/** 핸들로 현재 활성화된 액션의 런타임 정보를 검색합니다. */
	FActiveLuxAction* FindActiveAction(const FActiveLuxActionHandle& Handle);
	const FActiveLuxAction* FindActiveAction(const FActiveLuxActionHandle& Handle) const;

	/** 핸들로 현재 활성화된 액션의 런타임 정보를 검색합니다. */		
	FActiveLuxAction* FindActiveActionBySpecHandle(FLuxActionSpecHandle Handle);

	/** 예측 키를 생성하고 내부 카운터를 1 증가시킵니다. */
	FLuxPredictionKey CreatePredictionKey();

	/** 부여된 모든 액션 Spec의 배열을 const 참조로 반환합니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ActionSystem|Actions")
	const TArray<FLuxActionSpec>& GetActionSpecs() const;

	/** 현재 활성화된 모든 액션의 배열을 const 참조로 반환합니다. */
	const TArray<FActiveLuxAction>& GetActiveActions() const;
	/** 네이티브 이펙트 적용 알림 (서버: AddNewEffect, 클라: OnRep_EffectAdded) */
	FOnEffectAppliedNative OnEffectAppliedNative;
	/** 네이티브 이펙트 제거 알림 (서버: OnEffectExpired, 클라: OnRep_EffectRemoved) */
	FOnEffectRemovedNative OnEffectRemovedNative;

	/** 고유 액션 태그로 Spec을 찾습니다. 없으면 nullptr */
	FLuxActionSpec* FindActionSpecByIdentifierTag(const FGameplayTag& IdentifierTag);



private:
	/** ReHome 과정의 상세 정보를 디버그 로그로 출력합니다. */
	void LogReHomeDetails(const FActiveLuxAction& AuthoritativeAction, const ULuxAction* PredictedAction);

	/** 실패 태그에 맞는 알림 메시지를 화면에 표시합니다. */
	void ShowActionFailureMessage(const FGameplayTagContainer& FailureTags) const;

	// ======================================== Notification Delegates & Events ========================================
public:
	UPROPERTY(BlueprintAssignable, Category = "LuxActionSystem|Events")
	FOnActionActivated OnActionActivated;

	UPROPERTY(BlueprintAssignable, Category = "LuxActionSystem|Events")
	FOnActionFailed OnActionFailed;

	UPROPERTY(BlueprintAssignable, Category = "LuxActionSystem|Events")
	FOnActionEnded OnActionEnded;

protected:
	/** 클라이언트의 Task로부터 이벤트가 발생했음을 수신하는 RPC입니다. */
	UFUNCTION(Server, Reliable)
	void Server_ReceiveTaskEvent(FActiveLuxActionHandle ActionHandle, const FGameplayTag& EventTag, const FContextPayload& Payload);

	/** 
	 * 액션 활성화 성공을 알립니다.
	 * @param Handle 활성화된 액션의 고유 핸들입니다.
	 */
	virtual void NotifyActionActivated(const FActiveLuxActionHandle& Handle);

	/** 
	 * 액션 활성화 실패를 알립니다.
	 * @param Handle 활성화를 시도한 액션 Spec의 핸들입니다.
	 * @param FailureTags 실패 원인을 나타내는 태그 컨테이너입니다.
	 */
	virtual void NotifyActionFailed(const FLuxActionSpecHandle& Handle, const FGameplayTagContainer& FailureTags);

	/** 
	 * 액션 종료를 알립니다.
	 * @param Handle 종료된 액션의 고유 핸들입니다.
	 * @param bWasCancelled 취소되었는지 여부입니다.
	 */
	virtual void NotifyActionEnded(const FActiveLuxActionHandle& Handle, bool bWasCancelled);
#pragma endregion


#pragma region Effect Management
	/* ======================================== Effect Management ======================================== */
public:
	/** 새로운 이펙트 컨텍스트(Effect Context)를 생성합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	FLuxEffectContextHandle MakeEffectContext() const;

	/** 이펙트 클래스로부터 적용 가능한 이펙트 정보(Spec)를 생성합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	FLuxEffectSpecHandle MakeOutgoingSpec(TSubclassOf<ULuxEffect> EffectClass, float Level, FLuxEffectContextHandle& Context) const;

	/** 생성된 이펙트 Spec을 자기 자신에게 적용합니다. 성공 여부를 반환하고, 활성 핸들을 참조 인자로 돌려줍니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	bool ApplyEffectSpecToSelf(const FLuxEffectSpecHandle& SpecHandle, FActiveLuxEffectHandle& OutActiveHandle);

	/** 생성된 이펙트 Spec을 타겟에게 적용합니다. 성공 여부를 반환하고, 활성 핸들을 참조 인자로 돌려줍니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	bool ApplyEffectSpecToTarget(const FLuxEffectSpecHandle& SpecHandle, UActionSystemComponent* TargetASC, FActiveLuxEffectHandle& OutActiveHandle);

	/** 현재 적용 중인 이펙트를 제거합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	void RemoveEffect(FActiveLuxEffectHandle Handle);

	/** 현재 적용 중인 모든 이펙트를 제거합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	void RemoveAllActiveEffects();

	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	void RemoveActiveEffectsWithTags(const FGameplayTagContainer& TagsToRemove);

	/** 현재 활성화 목록에서 지정된 이펙트를 찾습니다.. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Effects")
	FActiveLuxEffectHandle FindActiveEffectHandleFromSpec(const FLuxEffectSpec& Spec);

	/** 기존에 활성화된 효과의 핸들을 찾습니다. */
	FActiveLuxEffectHandle FindExistingEffectHandle(const FLuxEffectSpec& AppliedSpec);

	/** 활성 효과 목록에서 지정된 Spec과 스택 가능한 기존 효과를 찾습니다. */
	FActiveLuxEffect* FindActiveEffectFromHandle(const FActiveLuxEffectHandle& Handle);

	/** [추가] 현재 활성화된 모든 이펙트의 배열을 const 참조로 반환합니다. */
	const TArray<FActiveLuxEffect>& GetActiveEffects() const;

private:
    /** 실제 이펙트 적용 로직을 처리하는 내부 함수입니다. 성공 여부를 반환하고, 지속 효과 핸들을 참조 인자로 돌려줍니다. */
    virtual bool ApplyEffectSpec_Internal(FLuxEffectSpec& Spec, FActiveLuxEffectHandle& OutActiveHandle);

	/** [1단계] Modifier 적용 전에 Execution 을 실행합니다. */
	bool PrepareSpecForApplication(FLuxEffectSpec& Spec);

	/** [2단계] AttributeSet의 전역 방어 로직을 실행합니다. */
	bool CheckApplicationPrerequisites(FLuxEffectSpec& Spec);

	/** [3단계] 최종 확정된 Modifier를 사용하여 속성을 실제로 변경합니다. */
	void ApplyModifiers(const FLuxEffectSpec& Spec);

	/** [4단계] 지속 효과를 활성 목록에 추가/갱신하고 타이머를 설정합니다. */
	FActiveLuxEffectHandle AddOrUpdateActiveEffect(const FLuxEffectSpec& AppliedSpec);

	/** 기존 효과를 업데이트합니다 (스택 증가, 지속시간 갱신 등). */
	FActiveLuxEffectHandle UpdateExistingEffect(FActiveLuxEffectHandle ExistingHandle, const FLuxEffectSpec& AppliedSpec);
	
	/** 새로운 효과를 추가합니다. */
	FActiveLuxEffectHandle AddNewEffect(const FLuxEffectSpec& AppliedSpec);
	
	/** 기존 효과의 타이머를 업데이트합니다. */
	void UpdateEffectTimers(FActiveLuxEffect* ExistingEffect, const FLuxEffectSpec& AppliedSpec);
	
	/** 새로운 효과의 타이머를 설정합니다. */
	void SetupEffectTimers(FActiveLuxEffect& NewActiveEffect, const FLuxEffectSpec& AppliedSpec);

	/** 이펙트의 만료 및 주기를 처리하는 타이머 콜백 함수입니다. */
	UFUNCTION()
	void OnEffectExpired(FActiveLuxEffectHandle Handle);
	UFUNCTION()
	void OnPeriodicEffectTick(FActiveLuxEffectHandle Handle);
#pragma endregion


#pragma region Cooldown Management
public:
    /** 서버 전용: 특정 쿨다운 태그의 남은 시간을 명시적으로 설정(타이머 재설정)합니다. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "LuxActionSystem|Cooldowns")
    void AdjustCooldownTimerByTag(const FGameplayTag& CooldownTag, float NewRemainingSeconds, float Now);

    /** 쿨다운 트래커에 대한 참조를 반환합니다. UI나 다른 시스템에서 직접 접근할 때 사용합니다. */
    UFUNCTION(BlueprintPure, Category = "LuxActionSystem|Cooldowns")
    ULuxCooldownTracker* GetCooldownTracker() const { return CooldownTracker; }

protected:
	void CreateCooldownTracker();

    /** 클라이언트에서 복제된 이펙트가 추가/제거될 때 쿨다운 트래커를 업데이트하기 위해 호출됩니다. */
    void OnRep_EffectAdded(const FActiveLuxEffect& NewEffect);
    void OnRep_EffectRemoved(const FActiveLuxEffect& RemovedEffect);
#pragma endregion


#pragma region Cue Management
public:
	/**
	 * 서버에서 게임플레이 큐를 실행하고 모든 클라이언트에게 복제하도록 요청합니다.
	 * @param CueTag 실행할 큐의 태그입니다.
	 * @param Context 큐 실행에 필요한 컨텍스트 데이터입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Cues")
	void ExecuteGameplayCue(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context);

protected:
	UFUNCTION(Server, Unreliable)
	void Server_ExecuteGameplayCue(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context, FLuxPredictionKey PredictionKey);

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_ExecuteCue(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context, FLuxPredictionKey PredictionKey);

	void ExecuteGameplayCue_Local(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context, FLuxPredictionKey& PredictionKey);

public:
	/**
	 * 서버에서 게임플레이 큐를 중지하고 모든 클라이언트에게 복제하도록 요청합니다.
	 * @param Target 큐를 중지할 대상 액터입니다.
	 * @param CueTag 중지할 큐의 태그입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Cues")
	void StopGameplayCue(AActor* Target, FGameplayTag CueTag);

protected:
	UFUNCTION(Server, Unreliable)
	void Server_StopGameplayCue(AActor* Target, FGameplayTag CueTag);

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_StopGameplayCue(AActor* Target, FGameplayTag CueTag);

	UFUNCTION(Client, Reliable)
	void Client_RejectCuePrediction(const FLuxPredictionKey& PredictionKey);

	UFUNCTION(BlueprintCallable)
	bool CanExecuteCue(FGameplayTag CueTag) const;
#pragma endregion


#pragma region Attribute Management
	/* ======================================== Attribute Management ======================================== */
public:
	/** 지정된 클래스의 어트리뷰트 셋을 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Attributes")
	ULuxAttributeSet* GetAttributeSet(TSubclassOf<ULuxAttributeSet> AttributeSetClass) const;

	/** 어트리뷰트 셋을 추가합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Attributes")
	void AddAttributeSet(ULuxAttributeSet* Attribute);

	/** 어트리뷰트 셋을 제거합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Attributes")
	void RemoveAttribute(ULuxAttributeSet* Attribute);

	/** 모든 어트리뷰트 셋을 제거합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Attributes")
	void RemoveAllAttributes();

	/** 지정된 Attribute의 CurrentValue를 강제적으로 수정합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Attributes")
	void ApplyModToAttribute(const FLuxAttribute& Attribute, EModifierOperation Operation, float Magnitude);

	/** 지정된 속성의 BaseValue를 설정합니다. */
	void SetNumericAttributeBase(const FLuxAttribute& Attribute, float NewBaseValue);

	/** 지정된 속성의 BaseValue를 반환합니다. */
	float GetNumericAttributeBase(const FLuxAttribute& Attribute) const;

	/** 속성 값 변경이 가능한지 미리 확인합니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LuxActionSystem|Attributes")
	bool CanApplyAttributeModifiers(const TArray<FAttributeModifier>& Modifiers, FGameplayTagContainer& OutFailureTags) const;

	const TArray<ULuxAttributeSet*>& GetSpawnedAttributes() const;

	template <class T>
	T* GetMutableAttributeSet()
	{
		return static_cast<T*>(GetAttributeSubobject(T::StaticClass()));
	}

	template <class T>
	const T* GetAttributeSet() const
	{
		return static_cast<const T*>(GetAttributeSubobject(T::StaticClass()));
	}

protected:
	ULuxAttributeSet* GetAttributeSubobject(TSubclassOf<ULuxAttributeSet> AttributeSetClass) const;
	ULuxAttributeSet* GetAttributeSubobjectChecked(const TSubclassOf<ULuxAttributeSet> AttributeSetClass) const;
	ULuxAttributeSet* GetOrCreateAttributeSet(TSubclassOf<ULuxAttributeSet> AttributeSetClass);

	/** 지정된 Attribute의 CurrentValue를 BaseValue와 모든 활성 효과를 바탕으로 재계산합니다. */
	void RecalculateCurrentAttributeValue(const FLuxAttribute& Attribute);

	/** 모든 부여된 Attribute들의 CurrentValue를 재계산합니다. */
	void RecalculateAllCurrentAttributeValues();

	void SetSpawnedAttributesDirty();
#pragma endregion


#pragma region Cooldown Management
/* ======================================== Cooldown Management ======================================== */
private:
    /** 쿨다운 전담 트래커 (Replicated Subobject) */
    UPROPERTY(Replicated)
    TObjectPtr<ULuxCooldownTracker> CooldownTracker;
#pragma endregion


#pragma region Input Management
	/* ======================================== Input Management ======================================== */
public:
	/** 입력 태그가 눌렸을 때 호출됩니다. */
	void ActionInputTagPressed(FGameplayTag InputTag);

	/** 입력 태그가 떼어졌을 때 호출됩니다. */
	void ActionInputTagReleased(FGameplayTag InputTag);

	/** 현재 모든 입력을 초기화합니다. */
	void ClearActionInput();

	/** 클라이언트에서 리플리케이트된 이벤트를 호출합니다. (내부적으로 RPC 전송) */
	void InvokeReplicatedEvent(EActionReplicatedEvent EventType, FLuxActionSpecHandle SpecHandle);

	/** 서버에서 이벤트를 수신하여 처리하는 RPC입니다. */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerInvokeReplicatedEvent(EActionReplicatedEvent EventType, FLuxActionSpecHandle SpecHandle);

	/** 로컬 입력이 눌렸을 때 브로드캐스트되는 델리게이트입니다. */
	UPROPERTY(BlueprintAssignable, Category = "LuxActionSystem|Input")
	FOnActionInputTagPressed OnLocalInputTagPressed;

	/** 로컬 입력이 떼어졌을 때 브로드캐스트되는 델리게이트입니다. */
	UPROPERTY(BlueprintAssignable, Category = "LuxActionSystem|Input")
	FOnActionInputTagReleased OnLocalInputTagReleased;

protected:
	/** 액션 Spec에 직접 입력 이벤트를 전달하는 내부 함수입니다. */
	void ActionInputTagPressed(FLuxActionSpec& Spec);
	void ActionInputTagReleased(FLuxActionSpec& Spec);

	/** 매 틱마다 지속적인 입력을 처리합니다. */
	void ProcessActionInput(float DeltaTime, bool bGamePaused);
#pragma endregion


#pragma region Tag Relationship
	/* ======================================== Tag Relationship ======================================== */
public:
	/** 태그 관계(차단/취소)를 정의하는 데이터 애셋을 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Relationship")
	void SetTagRelationshipMapping(ULuxActionTagRelationshipMapping* NewMapping);

	/** 서버에 태그 추가를 요청하는 클라이언트용 RPC 함수입니다. */
	UFUNCTION(Server, Reliable)
	void Server_AddTag(const FGameplayTag& Tag, int32 StackCount);

	/** 지정된 수만큼 태그 스택을 추가합니다. (StackCount가 1 미만이면 무시) */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Tags")
	void AddTag(const FGameplayTag& Tag, int32 StackCount);

	/** 컨테이너의 모든 태그에게 지정된 수만큼 스택을 추가합니다. (StackCount가 1 미만이면 무시) */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Tags")
	void AddTags(const FGameplayTagContainer& TagContainer, int32 StackCount);

	/** 서버에 태그 추가를 요청하는 클라이언트용 RPC 함수입니다. */
	UFUNCTION(Server, Reliable)
	void Server_RemoveTag(const FGameplayTag& Tag, int32 StackCount);

	/** 지정된 수만큼 태그 스택을 제거합니다. (StackCount가 1 미만이면 무시) */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Tags")
	void RemoveTag(const FGameplayTag& Tag, int32 StackCount);

	/** 컨테이너의 모든 태그에 대한 스택을 제거합니다. (StackCount가 1 미만이면 무시) */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Tags")
	void RemoveTags(const FGameplayTagContainer& TagContainer, int32 StackCount);

	/** 지정된 태그를 현재 가지고 있는지 확인합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Tags")
	bool HasTag(const FGameplayTag& Tag) const;

	/** 입력된 컨테이너의 태그 중 하나라도 가지고 있는지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "Lux|Tags")
	bool HasAny(const FGameplayTagContainer& TagContainer) const;

	/** 입력된 컨테이너의 모든 태그를 가지고 있는지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "Lux|Tags")
	bool HasAll(const FGameplayTagContainer& TagContainer) const;

	/** 지정된 태그의 현재 스택 수를 반환합니다. (없으면 0) */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Tags")
	int32 GetTagStackCount(const FGameplayTag& Tag) const;

	/** 컨테이너가 가진 모든 태그를 FGameplayTagContainer 형태로 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Tags")
	FGameplayTagContainer GetGameplayTags() const;

	/** 이 컴포넌트의 게임플레이 태그 스택 수가 변경될 때마다 호출됩니다. */
	UPROPERTY(BlueprintAssignable, Category = "LuxActionSystem|Tags")
	FOnGameplayTagStackChanged OnGameplayTagStackChanged;
#pragma endregion


#pragma region Gameplay Event System
	/* ======================================== Gameplay Event System ======================================== */
public:
	/**
	 * 게임플레이 이벤트를 이 컴포넌트에 방송합니다.
	 * @param EventTag 발생한 이벤트의 태그입니다.
	 * @param Payload 이벤트와 함께 전달할 추가 데이터입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Events")
	void HandleGameplayEvent(const FGameplayTag& EventTag, const FContextPayload& Payload);

	/** 특정 이벤트에 구독 */
    void SubscribeToGameplayEvent(const FGameplayTag& EventTag, const FScriptDelegate& Delegate);
    
	/** 특정 이벤트에 구독 해제 */
    void UnsubscribeFromGameplayEvent(const FGameplayTag& EventTag, const FScriptDelegate& Delegate);

    /** 이벤트 발생 시 구독자들에게만 전달 */
    void BroadcastGameplayEventToSubscribers(const FGameplayTag& EventTag, const FContextPayload& Payload);

private:
	/** 이벤트별 구독자 델리게이트: EventTag -> 델리게이트 */
    TMap<FGameplayTag, FOnGameplayEvent> EventSubscriptions;
#pragma endregion


#pragma region Core Properties
	/* ======================================== Core Properties ======================================== */
protected:
	/** 컴포넌트의 논리적 소유자(Owner)입니다. (예: APlayerState) */
	UPROPERTY(BlueprintReadOnly, Replicated)
	TWeakObjectPtr<AActor> OwnerActor;

	/** 컴포넌트 능력의 실제 사용자(Avatar)입니다. (예: AHeroCharacter) */
	UPROPERTY(BlueprintReadOnly, Replicated)
	TWeakObjectPtr<AActor> AvatarActor;

public:
	/** 태그 관계(차단/취소)를 정의하는 데이터 애셋입니다. */
	UPROPERTY()
	TObjectPtr<ULuxActionTagRelationshipMapping> TagRelationshipMapping;

#pragma endregion

#pragma region Core State Containers (Replicated)
	/* ======================================== Core State Containers (Replicated) ======================================== */
protected:
	/** 이 컴포넌트가 소유한 모든 액션 Spec 목록입니다. */
	UPROPERTY(Replicated)
	FActionSpecContainer LuxActionSpecs;

	/** 현재 활성화된 액션 목록입니다. */
	UPROPERTY(Replicated)
	FActiveLuxActionContainer ActiveLuxActions;

	/** 현재 적용 중인 모든 이펙트 목록입니다. */
	UPROPERTY(Replicated)
	FActiveLuxEffectsContainer ActiveLuxEffects;

	/** 이 컴포넌트에 부여된 모든 어트리뷰트 셋 목록입니다. */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<ULuxAttributeSet>> GreantedAttributes;

	/** 이 컴포넌트가 직접 소유한 게임플레이 태그 목록입니다. */
	UPROPERTY(Replicated)
	FGameplayTagStackContainer GrantedTags;

#pragma endregion

#pragma region Optimization Caches (Non-Replicated)
//protected:
//	friend struct FActiveLuxActionContainer;
//	friend struct FActiveLuxEffectsContainer;
//
//	/** FLuxActionSpec 포인터를 직접 찾기 위한 맵. (O(1) 조회용) */
//	TMap<FLuxActionSpecHandle, FLuxActionSpec*> ActionSpecMap;
//
//	/** FActiveLuxAction 포인터를 직접 찾기 위한 맵. */
//	TMap<FActiveLuxActionHandle, FActiveLuxAction*> ActiveActionMap;
//
//	/** FActiveLuxEffect 포인터를 직접 찾기 위한 맵. */
//	TMap<FActiveLuxEffectHandle, FActiveLuxEffect*> ActiveEffectMap;

	friend struct FActionSpecContainer;

	/** 해당 이벤트를 트리거로 사용하는 액션 핸들 목록을 저장하는 맵. (O(1) 조회용) */
	TMap<FGameplayTag, TArray<FLuxActionSpecHandle>> EventTriggerMap;
#pragma endregion

#pragma region Input Handling
protected:
	/** 로컬 입력 상태를 추적하기 위한 핸들 목록입니다. */
	TArray<FLuxActionSpecHandle> InputPressedSpecHandles;
	TArray<FLuxActionSpecHandle> InputReleasedSpecHandles;
	TArray<FLuxActionSpecHandle> InputHeldSpecHandles;

#pragma endregion

#pragma region Client-Side Prediction
private:
	/**
	 * 클라이언트가 예측 실행하고 서버의 확정/거절 응답을 기다리는 액션들의 핸들을 저장하는 Set입니다.
	 * 중복 RPC 전송을 막기 위한 클라이언트 측의 잠금 장치 역할을 합니다.
	 */
	TSet<FLuxActionSpecHandle> PendingPredictedActions;

	/** 클라이언트가 예측하고 서버의 응답을 기다리는 액션들을 추적하는 맵입니다. */
	UPROPERTY()
	TMap<FLuxPredictionKey, TObjectPtr<ULuxAction>> PendingPredictions;

	/** 클라이언트가 예측하고 서버의 응답을 기다리는 Cue들을 추적하는 맵입니다. */
	UPROPERTY()
	TMap<FLuxPredictionKey, FGameplayTag> PendingCues;

	/** 다음 예측에 사용할 키 ID입니다. */
	int32 NextPredictionKeyId = 1;

#pragma endregion

#pragma region Timer Management
private:
	/** 활성 이펙트의 만료 타이머를 관리하는 맵입니다. */
	TMap<FActiveLuxEffectHandle, FTimerHandle> ExpirationTimerMap;

	/** 활성 이펙트의 주기적인(Periodic) 실행 타이머를 관리하는 맵입니다. */
	TMap<FActiveLuxEffectHandle, FTimerHandle> PeriodTimerMap;
#pragma endregion



#pragma region Threading
private:
	/** 멀티스레드 환경에서 데이터 접근을 보호하기 위한 락(Lock)입니다. */
	mutable FRWLock ActionSpecsLock;
	mutable FRWLock ActiveActionsLock;
	mutable FRWLock ActiveLuxEffectsLock;
	mutable FRWLock InputHandlesLock;

#pragma endregion
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULuxAction>> PendingKillActions;

	bool bIsMoving = false;
};
