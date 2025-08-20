// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "LuxPayload.h"
#include "LuxActionTypes.h"
#include "LuxActionLevelData.h"
#include "LuxActionTags.h"
#include "Phase/LuxActionPhaseData.h"
#include "Phase/LuxActionPhaseTags.h"

#include "ActionSystem/Tasks/LuxActionTask.h"
#include "ActionSystem/LuxActionSystemTypes.h"
#include "System/GameplayTagStack.h"
#include "LuxAction.generated.h"





// 전방 선언
class UActionSystemComponent;
class ULuxActionCost;
class ULuxEffect;
class ULuxCameraMode;

struct FTaskEventData;
struct FPhaseTransition;
struct FActionPhaseData;

/**
 * 액션의 생명주기 상태를 나타내는 열거형
 * 액션이 현재 어떤 단계에 있는지 추적하는 데 사용됩니다.
 */
UENUM()
enum class ELuxActionLifecycleState : uint8
{
	/** 아직 활성화되지 않은 상태. 액션이 생성되었지만 실행되지 않음 */
	Inactive,

	/** 활성화되어 실행 중인 상태. 페이즈 전환이 일어나며 실제 게임플레이 로직이 실행됨 */
	Executing,

	/** 종료 절차 진행 중 (정리 페이즈 실행). 액션 종료를 위한 정리 작업 수행 */
	Ending,

	/** 모든 정리가 끝나고 완전히 종료된 상태. 액션 인스턴스가 정리될 준비가 됨 */
	Ended
};

/**
 * 페이즈 이름과 진행 단계를 함께 복제하기 위한 구조체
 * 네트워크 동기화를 위해 서버와 클라이언트 간 페이즈 상태를 동기화하는 데 사용됩니다.
 */
USTRUCT()
struct FReplicatedPhaseInfo
{
	GENERATED_BODY()

	/** 현재 실행 중인 페이즈의 태그 */
	UPROPERTY()
	FGameplayTag PhaseTag;

	/** 페이즈 전환 횟수를 추적하는 카운터. 클라이언트 예측과 서버 상태를 동기화하는 데 사용 */
	UPROPERTY()
	uint8 PhaseHistoryCounter = 0;
};

/**
 * 게임플레이 액션의 기본 클래스
 * 스킬, 공격, 이동 등 모든 게임플레이 액션의 기본이 되는 클래스입니다.
 * 페이즈 기반 실행 시스템을 통해 복잡한 액션 로직을 단계별로 관리합니다.
 */
UCLASS(Blueprintable)
class LUX_API ULuxAction : public UObject
{
	GENERATED_BODY()

	// 프렌드 클래스 선언
	friend class ALuxHUD;
	friend class UActionSystemComponent;
	friend class ULuxActionTask_WaitPhaseDelay;
	friend class ULuxActionTask_WaitMontageNotify;

	friend struct FActiveLuxActionContainer;
	friend struct FPhaseBehavior_RunTask;
	friend struct FPhaseBehavior_PushCameraMode;
	friend struct FPhaseBehavior_PopCameraMode;
	friend struct FDynamicFloat;

public:
	ULuxAction();
	virtual ~ULuxAction();

#pragma region UObject Overrides
	//~ UObject overrides
	/** 네트워크 복제 지원 여부를 반환합니다. 액션은 항상 네트워크 복제를 지원합니다. */
	FORCEINLINE bool IsSupportedForNetworking() const override { return true; }
	
	/** 서브오브젝트 복제를 처리합니다. 액션의 모든 서브오브젝트를 복제 대상으로 설정합니다. */
	FORCEINLINE virtual bool ReplicateSubobjects(class AActorChannel* Channel, class FOutBunch* Bunch, struct FReplicationFlags* RepFlags)
	{
		return true;
	}

	/** 복제 속성들의 수명을 정의합니다. 네트워크 동기화에 필요한 속성들을 등록합니다. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** 액션이 속한 월드를 반환합니다. 액션의 소유자 액터를 통해 월드에 접근합니다. */
	virtual UWorld* GetWorld() const override;
	//~ End of UObject overrides
#pragma endregion

#pragma region Getters & Accessors
public:
	/** 액션의 활성화 정책을 반환합니다. (즉시 실행, 입력 트리거 등) */
	UFUNCTION(BlueprintPure, Category = "LuxAction")
	ELuxActionActivationPolicy GetActivationPolicy() const { return ActivationPolicy; };

	/** 액션의 인스턴싱 정책을 반환합니다. (실행별 인스턴스, 공유 인스턴스 등) */
	UFUNCTION(BlueprintPure, Category = "LuxAction")
	ELuxActionInstancingPolicy GetInstancingPolicy() const { return InstancingPolicy; };

	/** 액션을 소유한 액터를 반환합니다. (플레이어 컨트롤러, AI 컨트롤러 등) */
	UFUNCTION(BlueprintCallable, Category = "LuxAction")
	AActor* GetOwnerActor() const;

	/** 액션을 실행하는 아바타 액터를 반환합니다. (캐릭터, 몬스터 등) */
	UFUNCTION(BlueprintCallable, Category = "LuxAction")
	AActor* GetAvatarActor() const;

	/** 현재 액션의 활성 핸들을 반환합니다. 액션 시스템에서 액션을 식별하는 데 사용됩니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxAction")
	FActiveLuxActionHandle GetActiveHandle() const;

	/** 액션을 관리하는 ActionSystemComponent를 반환합니다. 액션의 모든 시스템 기능에 접근할 수 있습니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxAction")
	UActionSystemComponent* GetActionSystemComponent() const;

	/** 현재 액션을 실행하는 아바타 액터의 네트워크 역할을 반환합니다. (서버, 클라이언트, 자율 프록시 등) */
	UFUNCTION(BlueprintPure, Category = "LuxAction|Network")
	ENetRole GetNetRole() const;

	/** 현재 실행 중인 액션의 레벨을 가져옵니다. 서버/클라이언트에 따라 ASC 또는 ActionPayload에서 데이터를 찾습니다. */
	UFUNCTION(BlueprintCallable, Category = "LuxAction")
	int32 GetActionLevel() const;

	/** 이 액션 인스턴스에 해당하는 FActiveLuxAction 구조체 포인터를 반환합니다. 액션의 활성 상태 정보에 접근할 수 있습니다. */
	FActiveLuxAction* GetActiveActionStruct() const;

	/** 이 액션의 원본 FLuxActionSpec 포인터를 반환합니다. 액션의 영구 데이터(설정, 태그 등)에 접근할 수 있습니다. */
	FLuxActionSpec* GetLuxActionSpec();
#pragma endregion

#pragma region Action Lifecycle Management
public:
	/** 액션 실행의 전체적인 흐름을 시작합니다. ActionSystemComponent에 의해 호출되며 액션의 메인 진입점입니다. */
	void ExecuteAction(const FOwningActorInfo& ActorInfo, const FLuxActionSpec& Spec, const FActiveLuxActionHandle& ActiveAction);

	/** 액션을 정상적으로 종료시키는 로직을 시작합니다. 정리 페이즈를 거쳐 안전하게 액션을 종료합니다. */
	void EndAction();

	/** 액션을 강제로 중단시키는 로직을 시작합니다. 즉시 종료되며 정리 작업을 수행하지 않습니다. */
	void CancelAction();

protected:
	/** 액션이 활성화될 때 비용 및 쿨다운을 적용합니다. 액션 실행 전 필요한 리소스 검사와 적용을 수행합니다. */
	virtual void ActivateAction(FLuxActionSpec& Spec, const FOwningActorInfo& ActorInfo);

	/** 액션의 모든 리소스를 정리하고 최종적으로 종료 처리합니다. 태그 제거, 태스크 정리, 메모리 해제 등을 수행합니다. */
	virtual void OnActionEnd(bool bIsCancelled);

	/**
	 * 클라이언트의 입력이 서버로 복제되었을 때 ActionSystemComponent에 의해 호출됩니다.
	 * 활성화된 액션이 추가 입력을 처리할 수 있도록 합니다. (예: 스킬 차징, 콤보 연계)
	 * @param EventType 발생한 입력 이벤트의 종류(Pressed, Released 등)
	 */
	virtual void OnReplicatedEvent(EActionReplicatedEvent EventType);

	/** OnReplicatedEvent의 블루프린트 버전입니다. 블루프린트에서 입력 이벤트를 처리할 수 있도록 노출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "LuxAction", meta = (DisplayName = "OnReplicatedEvent"))
	void K2_OnReplicatedEvent(EActionReplicatedEvent EventType);

public:
	/** 액션의 현재 생명주기 상태입니다. 액션이 현재 어떤 단계에 있는지 추적하는 데 사용됩니다. */
	UPROPERTY()
	ELuxActionLifecycleState LifecycleState = ELuxActionLifecycleState::Inactive;

	/** 액션의 레벨 데이터 테이블입니다. 액션의 레벨별 수치 데이터(데미지, 쿨다운 등)를 정의합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxAction|Lifecycle")
	TObjectPtr<UDataTable> LevelDataTable;

	/** 액션 실행에 필요한 컨텍스트 데이터를 저장하는 페이로드입니다. 액션의 실행 상태와 데이터를 관리합니다. */
	TSharedPtr<FContextPayload> ActionPayload;
#pragma endregion

#pragma region Phase System
protected:
	/* ===================== Core State & Control ===================== */

	/** 액션의 모든 페이즈 흐름을 정의하는 데이터 애셋입니다. 각 페이즈의 동작, 전환 조건, 효과 등을 정의합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxAction|Lifecycle")
	TObjectPtr<ULuxActionPhaseData> ActionPhaseData;

	/** 현재 실행 중인 페이즈의 태그입니다. 네트워크 복제되어 서버와 클라이언트 간 동기화됩니다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "LuxAction|Lifecycle")
	FGameplayTag CurrentPhaseTag;

	/** 페이즈 전환이 진행되는 동안 새로운 전환 요청이 들어오면 임시로 저장합니다. 중복 전환을 방지하는 데 사용됩니다. */
	UPROPERTY(Transient)
	FGameplayTag PendingNextPhaseTag;

	/** 페이즈 전환이 진행 중인지 나타내는 플래그입니다. 중복 전환을 방지하여 안정성을 보장합니다. */
	bool bIsTransitioningPhase = false;

	/** 지정된 이름의 페이즈(Phase)로 전환을 시작합니다. 페이즈 진입 로직과 전환 규칙 설정을 수행합니다. */
	virtual void EnterPhase(const FGameplayTag& NewPhaseTag, bool bForce);

	/** 현재 페이즈를 나가면서 필요한 정리 작업을 수행합니다. 전환 규칙 해제, 태스크 정리 등을 수행합니다. */
	virtual void ExitPhase();

	/* ===================== Phase Lifecycle Events ===================== */

	/** 새로운 페이즈에 진입했을 때 호출됩니다. 페이즈별 초기화 로직과 동작 설정을 수행합니다. */
	virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC);

	/** 현재 페이즈를 떠나기 직전에 호출됩니다. 페이즈별 정리 작업과 리소스 해제를 수행합니다. */
	virtual void OnPhaseExit(const FGameplayTag& PhaseTag);

	/** 블루프린트에서 각 페이즈 진입 시 추가적인 로직(파티클, 사운드 재생 등)을 처리할 수 있도록 노출된 이벤트입니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "LuxAction|Lifecycle", meta = (DisplayName = "On Phase Entered"))
	void K2_OnPhaseEntered(const FGameplayTag& PhaseTag, const UActionSystemComponent* SourceASC);

	/** 블루프린트에서 각 페이즈 종료 시 추가적인 로직을 처리할 수 있도록 노출된 이벤트입니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "LuxAction|Lifecycle", meta = (DisplayName = "On Phase Exited"))
	void K2_OnPhaseExited(const FGameplayTag& PhaseTag);

	/* ===================== Phase Transition ===================== */

	/** 페이즈 전환을 위한 통합된 전환 정보를 저장합니다. */
	struct FActivePhaseTransition
	{
		/** 전환 규칙 정보 */
		FPhaseTransition Transition;
		
		/** 구독된 델리게이트들 - 모든 타입을 FScriptDelegate로 통합 */
		FScriptDelegate GameplayEventDelegate;
		FScriptDelegate TaskEventDelegate;
		
		/** 전환 타입 */
		EPhaseTransitionType TransitionType;
		
		/** 이벤트 태그 */
		FGameplayTag EventTag;
		
		/** 델리게이트가 유효한지 확인 */
		bool HasValidDelegate() const
		{
			return (TransitionType == EPhaseTransitionType::OnGameplayEvent && GameplayEventDelegate.IsBound()) ||
				   (TransitionType == EPhaseTransitionType::OnTaskEvent && TaskEventDelegate.IsBound()) ||
				   (TransitionType == EPhaseTransitionType::OnDurationEnd && TaskEventDelegate.IsBound());
		}
	};

	/** 현재 활성화된 모든 페이즈 전환 정보를 저장합니다. */
	TArray<FActivePhaseTransition> ActivePhaseTransitions;

	/** C++ 코드에서 미리 정의된 페이즈 전환 규칙을 저장하는 맵입니다. 런타임에 동적으로 전환 규칙을 설정할 수 있습니다. */
	TMap<FGameplayTag, TArray<FPhaseTransition>> PhaseTransitionRules;

	/** 지정된 페이즈 데이터에 따라 다음 페이즈로의 전환 규칙들을 설정합니다. 이벤트 구독과 조건 설정을 수행합니다. */
	void SetupPhaseTransitions(const FActionPhaseData& PhaseData, UActionSystemComponent* ASC);

	/** 지정된 모든 전환 조건(Condition)들이 충족되는지 확인합니다. 모든 조건이 만족되어야 페이즈 전환이 가능합니다. */
	bool CheckAllConditions(const ULuxAction& Action, const TArray<FInstancedStruct>& Conditions, const FContextPayload& Payload);

	/** 현재 활성화된 모든 페이즈 전환 규칙과 이벤트 구독을 정리합니다. */
	void ClearPhaseTransitions();

	/** 통합된 페이즈 전환 이벤트 핸들러입니다. 모든 타입의 이벤트(게임플레이 이벤트, 태스크 이벤트, 딜레이 완료 이벤트, 완료 이벤트, 중단 이벤트)를 처리합니다. */
	UFUNCTION()
	void HandlePhaseTransitionEvent(const FGameplayTag& EventTag, const FContextPayload& Payload);

	/** 움직임 시작과 같은 범용 게임플레이 이벤트를 수신하여 중단 여부를 처리하는 핸들러입니다. 액션 중단 조건을 확인합니다. */
	UFUNCTION()
	void HandleGameplayEventForInterruption(const FGameplayTag& EventTag, const FContextPayload& Payload);

	/* ===================== Phase Behaviors & Effects ===================== */

	/** PhaseBehavior로 실행된 Cue 목록 (활성화/비활성화 관리용). 페이즈별 시각/청각 효과를 추적합니다. */
	UPROPERTY()
	TArray<FGameplayTag> ActivePhaseCues;

	/** 현재 페이즈에 진입할 때 정의되어 있는 PhaseBehavior들을 실행합니다. 태스크 생성, 태그 추가, 효과 적용 등을 수행합니다. */
	void ExecuteBehaviors(ULuxAction* Action, const TArray<FInstancedStruct>& Behaviors);

	/* ===================== Replication ===================== */

	/** 페이즈 상태(이름, 실행 카운터)를 복제하기 위한 구조체입니다. 서버와 클라이언트 간 페이즈 동기화에 사용됩니다. */
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedPhaseInfo)
	FReplicatedPhaseInfo ReplicatedPhaseInfo;

	/** 클라이언트 예측을 위한 로컬(복제되지 않는) 실행 카운터입니다. 클라이언트 예측과 서버 상태를 비교하는 데 사용됩니다. */
	uint8 LocalPhaseHistoryCounter = 0;

	/** ReplicatedPhaseInfo 복제 시 클라이언트에서 호출됩니다. 서버 상태에 맞춰 클라이언트 예측을 교정합니다. */
	UFUNCTION()
	void OnRep_ReplicatedPhaseInfo();
#pragma endregion



#pragma region Task & Event System
public:
	/** Task가 발생시킨 이벤트를 이 Action(게시판)에 게시(Broadcast)합니다. 태스크 간 통신과 페이즈 전환을 위한 이벤트 시스템입니다. */
	void PostTaskEvent(const FGameplayTag& EventTag, const FContextPayload& Payload);

	/** 외부에서 생성된 델리게이트를 이벤트에 등록합니다. 특정 이벤트 발생 시 호출될 함수를 등록할 수 있습니다. */
	void SubscribeToTaskEvent(const FGameplayTag& EventTag, const FScriptDelegate& InDelegate);

	/** 등록했던 델리게이트의 구독을 해제합니다. 메모리 누수를 방지하기 위해 더 이상 필요하지 않은 구독을 해제합니다. */
	void UnsubscribeFromTaskEvent(const FGameplayTag& EventTag, const FScriptDelegate& InDelegate);

	/** 새로운 Task가 이 액션에 의해 활성화되었을 때 호출됩니다. */
	virtual void OnTaskActivated(ULuxActionTask* Task);

	/** 활성화되었던 Task가 종료되었을 때 호출됩니다. 태스크 정리와 메모리 해제를 수행합니다. */
	virtual void OnTaskEnded(ULuxActionTask* Task);

	/** 클라이언트에서 예측 실행되던 Task의 소유권이 서버 복제본으로 이전되었을 때 호출됩니다. */
	virtual void OnTaskRehomed(ULuxActionTask* Task);

	/** 클라이언트의 Task로부터 이벤트가 발생했음을 서버에 알리기 위해 호출됩니다. */
	virtual void NotifyTaskEventToServer(const FGameplayTag& EventTag, const FContextPayload& Payload);

	/** '결과 저장 요청'이 있는 이벤트를 처리하는 범용 핸들러 함수입니다. */
	UFUNCTION()
	void HandleTaskResultStoringEvent(const FGameplayTag& EventTag, const FContextPayload& Payload);

public:
	/** 외부(Behavior)에서 '결과 저장 요청'을 등록하기 위한 함수입니다. 태스크 결과를 특정 키로 저장할 수 있도록 요청을 등록합니다. */
	void AddTaskResultStoreRequest(const FGameplayTag& EventTag, FName SourceKey, FName DestinationKey);

	/** 액션에 의해 스폰된 액터를 등록합니다. 액션 종료 시 함께 정리하기 위해 추적합니다. */
	void AddSpawnedActor(AActor* Actor);

protected:
	/** Task 결과를 저장하기 위한 요청 정보를 담는 내부 구조체입니다. 소스 키와 대상 키를 정의합니다. */
	struct FTaskResultStoreRequest
	{
		/** 결과 데이터의 소스 키 (태스크에서 제공하는 키) */
		FName SourceKey;
		/** 결과 데이터의 대상 키 (액션 컨텍스트에서 사용할 키) */
		FName DestinationKey;
	};

	/** 이벤트 태그별로 '결과 저장 요청' 목록을 관리하는 맵입니다. 각 이벤트 발생 시 저장할 결과들을 정의합니다. */
	TMap<FGameplayTag, TArray<FTaskResultStoreRequest>> TaskResultStoreRequests;

	/** 액션에 의해 스폰된 액터들의 목록입니다. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** 이벤트 태그를 Key로 핸들러 목록을 Value로 갖는 이벤트 핸들러 맵(게시판)입니다. */
	UPROPERTY()
	TMap<FGameplayTag, FLuxTaskEvent> EventHandlers;

	/** 현재 활성화된 모든 Task 객체의 목록입니다. */
	UPROPERTY()
	TArray<TObjectPtr<ULuxActionTask>> ActiveTasks;
#pragma endregion

#pragma region Tag Management System
public:
	/** 지정된 태그를 ASC에 추가하고 어떤 태그가 추가되었는지 기록합니다. 액션 실행 중 상태 관리를 위한 태그를 추가합니다. */
	void AddTag(const FGameplayTag& Tag, int32 StackCount);

	/** 지정된 태그를 ASC에서 제거하고 어떤 태그가 제거되었는지 기록합니다. 액션 종료 시 정리를 위한 태그를 제거합니다. */
	void RemoveTag(const FGameplayTag& Tag, int32 StackCount);

	/**
	 * 지정된 태그들을 ASC에 추가하고 어떤 태그가 추가되었는지 기록합니다.
	 * 이를 통해 액션 실행 주기에서 각 태그가 한 번만 추가되도록 합니다.
	 * @param TagsToAdd 추가할 게임플레이 태그 컨테이너입니다.
	 */
	void AddTags(const FGameplayTagContainer& TagsToAdd, int32 StackCount);

	/**
	 * 지정된 태그들을 ASC에서 제거하고, 어떤 태그가 제거되었는지 기록합니다.
	 * 이를 통해 액션 실행 주기에서 각 태그가 한 번만 제거되도록 합니다.
	 * @param TagsToRemove 제거할 게임플레이 태그 컨테이너입니다.
	 */
	void RemoveTags(const FGameplayTagContainer& TagsToRemove, int32 StackCount);

	/** 이 액션 인스턴스가 부여한 모든 태그들을 추적하기 위한 컨테이너입니다. 액션 종료 시 정리를 위해 사용됩니다. */
	FGameplayTagStackContainer GrantedTags;
#pragma endregion


#pragma region ReHome and State Transfer
public:
	/** 예측 액션의 상태를 현재 액션으로 이전합니다 (태스크, 카메라 모드, 태그, 스폰된 액터 등) */
	void TransferStateFrom(ULuxAction* PredictedAction);

protected:
	/** 이 액션이 ReHome 과정에서 상태 이전을 위해 파괴될 예정인지 여부 */
	UPROPERTY()
	bool bIsBeingReHomed = false;

public:
	/** ReHome 과정에서 상태 이전을 위한 플래그 설정 */
	void SetBeingReHomed(bool bInBeingReHomed) { bIsBeingReHomed = bInBeingReHomed; }

	/** ReHome 중인지 확인 */
	bool IsBeingReHomed() const { return bIsBeingReHomed; }
#pragma endregion


#pragma region Core Configuration Data
public:
	/** 액션의 인스턴스를 어떻게 생성하고 관리할지에 대한 정책입니다. (실행별 인스턴스, 공유 인스턴스 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "LuxAction")
	ELuxActionInstancingPolicy InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerExecution;

	/** 어빌리티의 활성화 방식을 정의합니다. (즉시 실행, 입력 트리거, 이벤트 트리거 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "LuxAction|Activation")
	ELuxActionActivationPolicy ActivationPolicy = ELuxActionActivationPolicy::OnInputTriggered;

	/** 액션 사용 시 소모될 비용을 정의하는 이펙트 클래스입니다. (페이즈와 무관하게 시작 시 적용) */
	UPROPERTY(EditDefaultsOnly, Category = "LuxAction|Cost")
	TSubclassOf<ULuxEffect> Cost;

	/** 액션 사용 시 적용될 쿨다운을 정의하는 이펙트 클래스입니다. (페이즈와 무관하게 시작 시 적용) */
	UPROPERTY(EditDefaultsOnly, Category = "LuxAction|Cooldown")
	TSubclassOf<ULuxEffect> Cooldown;

	/** 액션 시작 시 자동으로 쿨다운을 적용할지 여부를 나타냅니다. */
	UPROPERTY(EditDefaultsOnly, Category = "LuxAction|Cooldown")
	bool bApplyCooldownOnStart = true;

	/** 아이템 개수, 특수 스택 등 추가적인 비용입니다. 복합적인 비용 시스템을 지원합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "LuxAction|Cost")
	TArray<TSubclassOf<ULuxActionCost>> AdditionalCosts;

public:
	/**
	 * 이 액션을 고유하게 식별하는 태그입니다. (예: "Action.Aurora.Hoarfrost")
	 * 전투 로그 등에서 이 액션을 명확히 구분하는 데 사용됩니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags|Identity", meta = (Categories = "Action"))
	FGameplayTag ActionIdentifierTag;

	/** 액션의 종류를 정의하는 핵심 태그입니다. (예: "Action.Type.Melee", "Action.Type.Ability") */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Identity")
	FGameplayTagContainer ActionTags;

	/** GameplayEvent에 의해 액션이 트리거되는지 정의하는 태그들입니다. 이벤트 기반 액션 활성화를 지원합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Identity")
	FGameplayTagContainer EventTriggerTags;

	/** [시전자 조건] 액션을 활성화하기 위해 시전자가 반드시 가지고 있어야 하는 태그입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Owner Conditions")
	FGameplayTagContainer ActivationRequiredTags;

	/** [시전자 조건] 이 중 하나라도 시전자가 가지고 있으면 액션을 활성화할 수 없습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Owner Conditions")
	FGameplayTagContainer ActivationBlockedTags;

	/** [소스 조건] 이벤트로 활성화될 때 이벤트의 소스(Source)가 가지고 있어야 하는 태그입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Source Conditions")
	FGameplayTagContainer SourceRequiredTags;

	/** [소스 조건] 이벤트로 활성화될 때 소스(Source)가 이 중 하나라도 가지고 있으면 활성화할 수 없습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Source Conditions")
	FGameplayTagContainer SourceBlockedTags;

	/** [타겟 조건] 액션의 효과가 적용될 타겟이 가지고 있어야 하는 태그입니다. (예: 'State.Debuff.Burning') */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Target Conditions")
	FGameplayTagContainer TargetRequiredTags;

	/** [타겟 조건] 타겟이 이 중 하나라도 가지고 있으면 액션의 효과가 적용되지 않습니다. (예: 'State.Generic.Invulnerable') */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Target Conditions")
	FGameplayTagContainer TargetBlockedTags;

	/** 액션이 활성화될 때 지정된 태그의 액션들을 취소합니다. 중복 액션 방지와 우선순위 관리를 지원합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Tags|Target Conditions")
	FGameplayTagContainer CancelActionsWithTag;

	/** 이 컨테이너에 포함된 태그를 가진 이벤트는 서버로 전송됩니다. 네트워크 동기화가 필요한 이벤트를 정의합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Networking")
	FGameplayTagContainer ReplicatedEventTags;
#pragma endregion

#pragma region Core Properties & State
protected:
	/** 액션을 소유한 액터의 정보를 저장합니다. 소유자 식별과 권한 관리에 사용됩니다. */
	UPROPERTY(Replicated)
	FOwningActorInfo OwningActorInfo;

	/** 현재 액션의 활성 핸들을 저장합니다. 액션 시스템에서 액션을 식별하고 관리하는 데 사용됩니다. */
	UPROPERTY(Replicated)
	FActiveLuxActionHandle ActiveActionHandle;

	/** 이 액션의 페이즈에 의해 활성화된 카메라 모드들의 스택입니다. 액션별 카메라 제어를 관리합니다. */
	UPROPERTY(Transient)
	TArray<TSubclassOf<ULuxCameraMode>> PushedCameraModes;

	/** 이벤트 핸들러 맵에 대한 동시 접근을 막기 위한 크리티컬 섹션입니다. 스레드 안전성을 보장합니다. */
	mutable FCriticalSection EventHandlersCS;
#pragma endregion

#pragma region UI & Display Data
public:
	/** 액션의 표시 이름입니다. UI에서 사용자에게 보여지는 이름을 정의합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxAction|UI")
	FText DisplayName;

	/** 액션의 설명입니다. UI에서 사용자에게 보여지는 상세 설명을 정의합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxAction|UI", meta = (MultiLine = true))
	FText Description;

	/** 액션의 아이콘입니다. UI에서 사용자에게 보여지는 시각적 아이콘을 정의합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxAction|UI")
	TSoftObjectPtr<UTexture2D> Icon;
#pragma endregion

#pragma region Utility & Debug
public:
	/** 로그 메시지에 [Server/Client] [Action Name] 접두사를 붙여주는 헬퍼 함수입니다. 디버깅과 로그 추적에 사용됩니다. */
	FString GetLogPrefix() const;
	
	/** 클라이언트/서버 구분을 위한 문자열을 저장합니다. 로그 메시지 생성 시 사용됩니다. */
	mutable FString ClientServerString;
#pragma endregion
};


#define SUBSCRIBE_TO_GAMEPLAY_EVENT(EventTag, FunctionName) \
    { \
		UActionSystemComponent* OwningASC = GetActionSystemComponent(); \
		if(ensure(OwningASC)) \
		{ \
			FScriptDelegate Delegate; \
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, FunctionName)); \
			OwningASC->SubscribeToGameplayEvent(EventTag, Delegate); \
		} \
    }

#define UNSUBSCRIBE_FROM_GAMEPLAY_EVENT(EventTag, FunctionName) \
    { \
		UActionSystemComponent* OwningASC = GetActionSystemComponent(); \
		if(ensure(OwningASC)) \
		{ \
			FScriptDelegate Delegate; \
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, FunctionName)); \
			OwningASC->UnsubscribeFromGameplayEvent(EventTag, Delegate); \
		} \
    }

#define SUBSCRIBE_TO_TASK_EVENT(EventTag, FunctionName) \
    { \
        FScriptDelegate Delegate; \
        Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, FunctionName)); \
        SubscribeToTaskEvent(EventTag, Delegate); \
    }

#define UNSUBSCRIBE_FROM_TASK_EVENT(EventTag, FunctionName) \
    { \
        FScriptDelegate Delegate; \
        Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, FunctionName)); \
        UnsubscribeFromTaskEvent(EventTag, Delegate); \
    }