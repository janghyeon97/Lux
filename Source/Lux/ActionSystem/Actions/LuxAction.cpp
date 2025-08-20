// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxActionCost.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Effects/LuxEffect.h"

#include "ActionSystem/Tasks/LuxActionTask_WaitPhaseDelay.h"
#include "ActionSystem/Tasks/LuxActionTask_PlayMontageAndWait.h"
#include "Phase/LuxActionPhaseTags.h"

#include "Character/LuxHeroCharacter.h"
#include "Camera/LuxCameraComponent.h" 

#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

#include "HAL/PlatformProcess.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"

/* ======================================== 생성자 및 UObject 오버라이드 ======================================== */

ULuxAction::ULuxAction()
{
	InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerExecution;
	ActivationPolicy = ELuxActionActivationPolicy::OnInputTriggered;
	LifecycleState = ELuxActionLifecycleState::Inactive;

	ActiveTasks.Reset();

	static ConstructorHelpers::FClassFinder<ULuxEffect> CostEffectFinder(TEXT("/Game/LuxEffects/LE_GenericCost_C.LE_GenericCost_C"));
	if (CostEffectFinder.Succeeded()) Cost = CostEffectFinder.Class;
	else Cost = nullptr;

	static ConstructorHelpers::FClassFinder<ULuxEffect> CooldownEffectFinder(TEXT("/Game/LuxEffects/LE_GenericCooldown.LE_GenericCooldown"));
	if (CooldownEffectFinder.Succeeded()) Cooldown = CooldownEffectFinder.Class;
	else Cooldown = nullptr;
}

ULuxAction::~ULuxAction()
{
	// 소멸자에서 모든 태스크 이벤트 핸들러(게시판)를 정리합니다.
	FScopeLock Lock(&EventHandlersCS);

	for (auto& Pair : EventHandlers)
	{
		Pair.Value.Clear();
	}

	EventHandlers.Empty();
}

//bool ULuxAction::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
//{
//	AActor* Owner = Cast<AActor>(GetOuter());
//	if (!Owner) return false;
//
//	UNetDriver* NetDriver = Owner->GetNetDriver();
//	if (!NetDriver) return false;
//
//	NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
//	return true;
//
//}
//
//int32 ULuxAction::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
//{
//	AActor* Owner = Cast<AActor>(GetOuter());
//	return (Owner ? Owner->GetFunctionCallspace(Function, Stack) : FunctionCallspace::Local);
//
//}

void ULuxAction::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULuxAction, OwningActorInfo);
	DOREPLIFETIME(ULuxAction, ActiveActionHandle);
	DOREPLIFETIME(ULuxAction, InstancingPolicy);
	DOREPLIFETIME(ULuxAction, ActivationPolicy);

	DOREPLIFETIME_CONDITION(ULuxAction, CurrentPhaseTag, COND_None);
	DOREPLIFETIME_CONDITION(ULuxAction, ReplicatedPhaseInfo, COND_None);
}

UWorld* ULuxAction::GetWorld() const
{
	UActorComponent* Component = Cast<UActorComponent>(GetOuter());
	if (Component)
	{
		return Component->GetWorld();
	}
	return nullptr;
}

/* ======================================== Getters ======================================== */

AActor* ULuxAction::GetOwnerActor() const
{
	if (InstancingPolicy == ELuxActionInstancingPolicy::NonInstanced)
	{
		return nullptr;
	}

	return OwningActorInfo.OwnerActor.Get();
}

AActor* ULuxAction::GetAvatarActor() const
{
	if (InstancingPolicy == ELuxActionInstancingPolicy::NonInstanced)
	{
		return nullptr;
	}

	return OwningActorInfo.AvatarActor.Get();
}

FActiveLuxActionHandle ULuxAction::GetActiveHandle() const
{
	if (InstancingPolicy == ELuxActionInstancingPolicy::NonInstanced)
	{
		return FActiveLuxActionHandle();
	}

	return ActiveActionHandle;
}

UActionSystemComponent* ULuxAction::GetActionSystemComponent() const
{
	if (InstancingPolicy == ELuxActionInstancingPolicy::NonInstanced)
	{
		return nullptr;
	}

	return OwningActorInfo.ActionSystemComponent.Get();
}

ENetRole ULuxAction::GetNetRole() const
{
	const AActor* Avatar = GetAvatarActor();
	return Avatar ? Avatar->GetLocalRole() : ENetRole::ROLE_None;
}

int32 ULuxAction::GetActionLevel() const
{
	// ASC에서 ActiveAction 데이터를 먼저 검색합니다.
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (ASC)
	{
		if (const FActiveLuxAction* FoundAction = ASC->FindActiveAction(GetActiveHandle()))
		{
			return FoundAction->Spec.Level;
		}
	}

	// 실패 시 (예측 클라이언트인 경우), ActionPayload에서 Spec 데이터를 찾아 레벨을 반환합니다.
	if (ActionPayload.IsValid())
	{
		if (const FLuxActionSpec* SpecFromPayload = ActionPayload->GetData<FLuxActionSpec>(FName("ActionSpec")))
		{
			return SpecFromPayload->Level;
		}
	}

	return 1;
}

FActiveLuxAction* ULuxAction::GetActiveActionStruct() const
{
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC)
	{
		return nullptr;
	}

	return ASC->FindActiveAction(GetActiveHandle());
}

FLuxActionSpec* ULuxAction::GetLuxActionSpec()
{
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC)
	{
		return nullptr;
	}

	if (const FActiveLuxAction* ActiveAction = GetActiveActionStruct())
	{
		if (FLuxActionSpec* SpecFromASC = ASC->FindActionSpecFromHandle(ActiveAction->Spec.Handle))
		{
			return SpecFromASC;
		}
	}

	if (ActionPayload.IsValid() && ActionPayload->HasData(LuxPayloadKeys::ActionSpec))
	{
		return const_cast<FLuxActionSpec*>(ActionPayload->GetData<FLuxActionSpec>(LuxPayloadKeys::ActionSpec));
	}

	return nullptr;
}

/* ======================================== Action Lifecycle ======================================== */

void ULuxAction::ActivateAction(FLuxActionSpec& Spec, const FOwningActorInfo& ActorInfo)
{
	UActionSystemComponent* ASC = ActorInfo.ActionSystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	FLuxEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.SetTargetASC(ASC);
	EffectContext.SetSourceAction(Spec.Handle);

	if (!LevelDataTable)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] 액션 [%s]: LevelDataTable이 설정되지 않았습니다."), *ClientServerString, *GetName());
		return;
	}

	const FName RowName = FName(*FString::FromInt(Spec.Level));
	const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("ActivateAction"));
	if (!LevelData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] 액션 [%s]: 레벨 데이터 [%s]을(를) 찾을 수 없습니다."), *ClientServerString, *GetName(), *RowName.ToString());
		return;
	}

	const FActionLevelDataBase* ActionLevelData = LevelData->ActionSpecificData.GetPtr<FActionLevelDataBase>();
	if (!ActionLevelData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] 액션 [%s]: Level '%d' 의 LevelData에서 ActionLevelData 타입의 데이터를 찾을 수 없습니다."), *ClientServerString, *GetName(), Spec.Level);
		return;
	}

	// 비용(Cost) 효과 적용
	if (Cost)
	{
		FLuxEffectSpecHandle CostSpecHandle = ASC->MakeOutgoingSpec(Cost, Spec.Level, EffectContext);
		if (CostSpecHandle.IsValid())
		{
			FActiveLuxEffectHandle Tmp; ASC->ApplyEffectSpecToSelf(CostSpecHandle, Tmp);
		}
	}

	// 추가 비용(Additional Costs) 적용
	for (TSubclassOf<ULuxActionCost> CostClass : AdditionalCosts)
	{
		ULuxActionCost* CostCDO = CostClass->GetDefaultObject<ULuxActionCost>();
		if (CostCDO)
		{
			CostCDO->ApplyCost(ASC, Spec);
		}
	}

	// 쿨다운(Cooldown) 효과 적용
	if (bApplyCooldownOnStart && Cooldown)
	{
		FLuxEffectSpecHandle CooldownSpecHandle = ASC->MakeOutgoingSpec(Cooldown, Spec.Level, EffectContext);
		if (CooldownSpecHandle.IsValid())
		{
			FActiveLuxEffectHandle Tmp; ASC->ApplyEffectSpecToSelf(CooldownSpecHandle, Tmp);
		}
	}
}

void ULuxAction::ExecuteAction(const FOwningActorInfo& ActorInfo, const FLuxActionSpec& Spec, const FActiveLuxActionHandle& ActiveAction)
{
	// 현재 컨텍스트를 미리 저장해두어 로그에 사용합니다.
	ClientServerString = GetClientServerContextString(ActorInfo.AvatarActor.Get());

	// 인스턴스화 정책(InstancingPolicy)에 따라 실행 흐름이 결정됩니다.
	UActionSystemComponent* SourceASC = ActorInfo.ActionSystemComponent.Get();
	if (::IsValid(SourceASC) == false || SourceASC->IsValidLowLevel() == false || SourceASC->IsRegistered() == false)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] Action [%s]: ExecuteAction 실패 - 유효하지 않은 액션 시스템 컴포넌트입니다."), *ClientServerString, *GetName());
		return;
	}

	if (LifecycleState == ELuxActionLifecycleState::Executing || LifecycleState == ELuxActionLifecycleState::Ending)
	{
		return;
	}

	// NonInstanced 정책일 경우
	if (InstancingPolicy == ELuxActionInstancingPolicy::NonInstanced)
	{
		TSharedPtr<FContextPayload> InstantPayload = MakeShared<FContextPayload>();
		InstantPayload->SetData(LuxPayloadKeys::ActionSpec, Spec);

		OnPhaseEnter(LuxPhaseTags::Phase_Action_Begin, *SourceASC);

		FContextPayload BpOutPayload = *InstantPayload;
		K2_OnPhaseEntered(LuxPhaseTags::Phase_Action_Begin, SourceASC);

		// NonInstanced 액션은 한 번 실행 후 바로 종료됩니다.
		SourceASC->OnActionEnd(ActiveAction, false);
		return;
	}

	OwningActorInfo = ActorInfo;
	ActiveActionHandle = ActiveAction;
	LifecycleState = ELuxActionLifecycleState::Executing;

	if (::IsValid(ActionPhaseData) && ActionPhaseData->Phases.Num() > 0)
	{
		// 액션 페이로드 데이터를 설정합니다.
		ActionPayload = MakeShared<FContextPayload>();
		ActionPayload->SetData(LuxPayloadKeys::ActionSpec, Spec);

		// 초기 페이즈로 진입합니다.
		const FGameplayTag InitialPhaseTag = ActionPhaseData->Phases.CreateConstIterator()->Key;
		EnterPhase(InitialPhaseTag, true);
	}
	else
	{
		// 정의된 페이즈가 없으면 액션을 즉시 종료합니다.
		EndAction();
	}
}

void ULuxAction::EndAction()
{
	if (LifecycleState != ELuxActionLifecycleState::Executing) return;
	LifecycleState = ELuxActionLifecycleState::Ending;

	OnActionEnd(false);

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (ASC)
	{
		ASC->OnActionEnd(GetActiveHandle(), false);
	}
}

void ULuxAction::CancelAction()
{
	if (LifecycleState != ELuxActionLifecycleState::Executing) return;
	LifecycleState = ELuxActionLifecycleState::Ending;

	OnActionEnd(true);

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (ASC)
	{
		ASC->OnActionEnd(GetActiveHandle(), true);
	}
}

void ULuxAction::OnActionEnd(bool bIsCancelled)
{
	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] Action [%s] OnActionEnd 호출, bIsCancelled: %s, bIsBeingReHomed: %s"), 
		*ClientServerString, *GetName(), bIsCancelled ? TEXT("true") : TEXT("false"), bIsBeingReHomed ? TEXT("true") : TEXT("false"));

	// 이미 종료된 액션의 중복 실행을 방지합니다.
	if (LifecycleState == ELuxActionLifecycleState::Ended)
	{
		return;
	}

	// 예측 액션이 ReHome될 예정이라면 상태 정리를 하지 않습니다.
	// 상태는 권위적인 액션으로 이전되고, 권위적인 액션이 나중에 정리를 담당합니다.
	if (bIsBeingReHomed)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] Action [%s] ReHome 대상이므로 상태 정리를 건너뜁니다."), *ClientServerString, *GetName());
		
		// 현재 활성화된 페이즈만 정리하고, 나머지 리소스는 그대로 둡니다.
		ExitPhase();
		
		// 태스크 이벤트 핸들러만 정리
		{
			FScopeLock Lock(&EventHandlersCS);
			if (EventHandlers.Num() > 0)
			{
				for (auto& Pair : EventHandlers)
				{
					Pair.Value.Clear();
				}
				EventHandlers.Empty();
			}
		}
		
		// 라이프사이클 상태만 변경하고 리턴
		LifecycleState = ELuxActionLifecycleState::Ended;
		return;
	}

	// 현재 활성화된 페이즈의 모든 리소스를 먼저 정리합니다.
	ExitPhase();

	// 모든 태스크 이벤트 핸들러(게시판)를 정리합니다.
	{
		FScopeLock Lock(&EventHandlersCS);

		if (EventHandlers.Num() > 0)
		{
			for (auto& Pair : EventHandlers)
			{
				Pair.Value.Clear();
			}

			EventHandlers.Empty();
		}
	}

	// 페이즈 전환과 관계없이 활성화된 나머지 모든 태스크를 종료시킵니다.
	if (ActiveTasks.Num() > 0)
	{
		const TArray<TObjectPtr<ULuxActionTask>> TasksToEnd = ActiveTasks;
		for (ULuxActionTask* Task : TasksToEnd)
		{
			if (Task)
			{

				Task->EndTask(false);
			}
		}

		ActiveTasks.Empty();
	}

	// 액션 전반에 걸쳐 사용된 데이터를 정리합니다.
	ActionPayload.Reset();

	// 액션에 의해 생성된 모든 액터를 정리합니다.
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor) Actor->Destroy();
	}
	SpawnedActors.Empty();


	if (ALuxHeroCharacter* Hero = Cast<ALuxHeroCharacter>(GetAvatarActor()))
	{
		if (ULuxCameraComponent* CameraComponent = Hero->GetCameraComponent())
		{
			// Push된 횟수만큼 Pop을 호출하여 스택을 원래대로 되돌립니다.
			for (int32 i = 0; i < PushedCameraModes.Num(); ++i)
			{
				CameraComponent->PopCameraMode();
			}
		}
	}
	PushedCameraModes.Empty();

	// 6. 아직 제거되지 않은 태그들을 정리합니다.
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (ASC)
	{
		FGameplayTagContainer TagsToRemove = GrantedTags.GetExplicitGameplayTags();
		if (TagsToRemove.Num() > 0)
		{
			for (const FGameplayTag& Tag : TagsToRemove)
			{
				ASC->RemoveTag(Tag, GrantedTags.GetStackCount(Tag));
			}

		}
	}

	// 플래그 초기화
	bIsTransitioningPhase = false;

	LifecycleState = ELuxActionLifecycleState::Ended;

}

void ULuxAction::OnReplicatedEvent(EActionReplicatedEvent EventType)
{
	K2_OnReplicatedEvent(EventType);
}

/* ======================================== Phase Logic ======================================== */

void ULuxAction::EnterPhase(const FGameplayTag& NewPhaseTag, bool bForce)
{
	// 페이즈 전환 중복 실행 방지
	if (bIsTransitioningPhase && !bForce)
	{
		return;
	}

	// 목표 페이즈가 현재 페이즈와 동일하다면 요청을 무시합니다.
	if (NewPhaseTag == CurrentPhaseTag && !bForce)
	{
		return;
	}

	// 현재 다른 페이즈로 전환 중이라면 새로운 요청을 저장하고 즉시 종료합니다.
	if (bIsTransitioningPhase)
	{
		PendingNextPhaseTag = NewPhaseTag;
		return;
	}

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (::IsValid(ASC) == false || NewPhaseTag.IsValid() == false)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] Action [%s] tried to enter an invalid phase [%s]. Ending action."), *ClientServerString, *GetName(), *NewPhaseTag.ToString());
		EndAction();
		return;
	}

	if (::IsValid(ActionPhaseData) == false || !ActionPhaseData->Phases.Contains(NewPhaseTag))
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] Action [%s] tried to enter undefined phase [%s]."), *ClientServerString, *GetName(), *NewPhaseTag.ToString());
		EndAction();
		return;
	}

	bIsTransitioningPhase = true;

	// 서버와 클라이언트의 페이즈 진행 상태를 동기화합니다.
	if (GetAvatarActor() && GetAvatarActor()->HasAuthority())
	{
		ReplicatedPhaseInfo.PhaseHistoryCounter++;
		ReplicatedPhaseInfo.PhaseTag = NewPhaseTag;
	}
	else
	{
		LocalPhaseHistoryCounter++;
	}

	// 이전 페이즈의 모든 리소스(이벤트 구독, 타이머 등)를 정리합니다.
	ExitPhase();

	CurrentPhaseTag = NewPhaseTag;

	const FActionPhaseData& PhaseData = ActionPhaseData->Phases[NewPhaseTag];

	// 움직임에 의한 중단이 가능한지 확인합니다.
	if (PhaseData.bCanAnimationBeInterruptedByMovement)
	{
		SUBSCRIBE_TO_GAMEPLAY_EVENT(LuxGameplayTags::Event_Movement_Started, HandleGameplayEventForInterruption);
	}

	// 이벤트를 발생시킬 수 있는 Behavior 들을 실행합니다.
	ExecuteBehaviors(this, PhaseData.OnEnterBehaviors);

	// 특정 태그를 가진 다른 액션들을 취소시킵니다.
	if (!PhaseData.CancelActionsWithTag.IsEmpty())
	{
		ASC->CancelActions(&PhaseData.CancelActionsWithTag, nullptr);
	}

	OnPhaseEnter(CurrentPhaseTag, *ASC);
	K2_OnPhaseEntered(CurrentPhaseTag, ASC);

	bIsTransitioningPhase = false; // 페이즈 전환 완료

	// 새로운 페이즈의 전환 규칙을 설정합니다.
	SetupPhaseTransitions(PhaseData, ASC);
}

void ULuxAction::ExitPhase()
{
	// 이미 나간 상태이므로 추가 작업을 방지합니다.
	if (!CurrentPhaseTag.IsValid())
	{
		return;
	}

	// 액션 컴포넌트가 유효하지 않으면 액션을 즉시 종료합니다.
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC)
	{
		EndAction();
		return;
	}

	// 데이터 애셋에 요청된 페이즈가 정의되어 있는지 확인합니다.
	if (!ActionPhaseData || !ActionPhaseData->Phases.Contains(CurrentPhaseTag))
	{
		EndAction();
		return;
	}

	const FActionPhaseData& PhaseData = ActionPhaseData->Phases[CurrentPhaseTag];

	ClearPhaseTransitions();
	ExecuteBehaviors(this, PhaseData.OnExitBehaviors);
	OnPhaseExit(CurrentPhaseTag);
	K2_OnPhaseExited(CurrentPhaseTag);

	TaskResultStoreRequests.Empty();
	CurrentPhaseTag = FGameplayTag::EmptyTag;
}

void ULuxAction::SetupPhaseTransitions(const FActionPhaseData& PhaseData, UActionSystemComponent* ASC)
{
	// 기존 전환 규칙과 이벤트 구독을 모두 정리합니다.
	ClearPhaseTransitions();

	// 데이터 애셋과 C++ 코드에 정의된 규칙을 모두 담을 임시 배열을 생성합니다.
	TArray<FPhaseTransition> CombinedTransitions = PhaseData.Transitions;
	if (const TArray<FPhaseTransition>* CppDefinedTransitions = PhaseTransitionRules.Find(CurrentPhaseTag))
	{
		CombinedTransitions.Append(*CppDefinedTransitions);
	}

	UE_LOG(LogLuxActionSystem, Log, TEXT("%s페이즈 [%s]의 전환 규칙 설정을 시작합니다."), *GetLogPrefix(), *CurrentPhaseTag.ToString());

	// 통합된 모든 전환 규칙을 순회하며 설정합니다.
	for (const FPhaseTransition& Transition : CombinedTransitions)
	{
		if (!Transition.NextPhaseTag.IsValid())
		{
			continue;
		}

		// 조건(Condition) 목록을 문자열로 포맷합니다.
		FString ConditionsString;
		if (Transition.Conditions.Num() > 0)
		{
			ConditionsString = TEXT(" (Conditions: ");
			for (const FInstancedStruct& ConditionStruct : Transition.Conditions)
			{
				if (!ConditionStruct.IsValid()) continue;

				ConditionsString += ConditionStruct.GetScriptStruct()->GetName();
				ConditionsString += TEXT(", ");
			}
			ConditionsString.LeftChopInline(2);
			ConditionsString += TEXT(")");
		}

		// 전환 유형에 따라 처리합니다.
		switch (Transition.TransitionType)
		{
		case EPhaseTransitionType::Immediate:
			UE_LOG(LogLuxActionSystem, Log, TEXT("    -> 즉시 [%s](으)로 전환합니다."), *Transition.NextPhaseTag.ToString());
			EnterPhase(Transition.NextPhaseTag, false);
			return;

		case EPhaseTransitionType::OnDurationEnd:
		{
			UE_LOG(LogLuxActionSystem, Log, TEXT("    -> %.2f초 후 [%s](으)로 전환합니다."), Transition.Duration, *Transition.NextPhaseTag.ToString());

			// 딜레이 태스크 생성
			ULuxActionTask_WaitPhaseDelay::WaitPhaseDelay(this, Transition.Duration);

			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxAction, HandlePhaseTransitionEvent));
			SubscribeToTaskEvent(LuxGameplayTags::Task_Event_PhaseDelay_Finished, Delegate);

			// 통합된 전환 정보 저장
			FActivePhaseTransition& ActiveTransition = ActivePhaseTransitions.AddDefaulted_GetRef();
			ActiveTransition.Transition = Transition;
			ActiveTransition.TaskEventDelegate = Delegate;
			ActiveTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
			ActiveTransition.EventTag = LuxGameplayTags::Task_Event_PhaseDelay_Finished;
			break;
		}

		case EPhaseTransitionType::OnGameplayEvent:
		{
			UE_LOG(LogLuxActionSystem, Log, TEXT("    -> GameplayEvent [%s] 발생 시 [%s](으)로 전환합니다.%s"), *Transition.EventTag.ToString(), *Transition.NextPhaseTag.ToString(), *ConditionsString);

			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxAction, HandlePhaseTransitionEvent));
			ASC->SubscribeToGameplayEvent(Transition.EventTag, Delegate);

			FActivePhaseTransition& ActiveTransition = ActivePhaseTransitions.AddDefaulted_GetRef();
			ActiveTransition.Transition = Transition;
			ActiveTransition.GameplayEventDelegate = Delegate;
			ActiveTransition.TransitionType = EPhaseTransitionType::OnGameplayEvent;
			ActiveTransition.EventTag = Transition.EventTag;
			break;
		}

		case EPhaseTransitionType::OnTaskEvent:
		{
			UE_LOG(LogLuxActionSystem, Log, TEXT("    -> TaskEvent [%s] 발생 시 [%s](으)로 전환합니다.%s"), *Transition.EventTag.ToString(), *Transition.NextPhaseTag.ToString(), *ConditionsString);

			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxAction, HandlePhaseTransitionEvent));
			SubscribeToTaskEvent(Transition.EventTag, Delegate);

			FActivePhaseTransition& ActiveTransition = ActivePhaseTransitions.AddDefaulted_GetRef();
			ActiveTransition.Transition = Transition;
			ActiveTransition.TaskEventDelegate = Delegate;
			ActiveTransition.TransitionType = EPhaseTransitionType::OnTaskEvent;
			ActiveTransition.EventTag = Transition.EventTag;
			break;
		}
		case EPhaseTransitionType::Manual:
			break;
		}
	}
}

void ULuxAction::ClearPhaseTransitions()
{
	if (ActiveTasks.Num() > 0)
	{
		TArray<ULuxActionTask*> TasksToRemove;
		for (ULuxActionTask* Task : ActiveTasks)
		{
			if (Task && Task->IsA<ULuxActionTask_WaitPhaseDelay>())
			{
				TasksToRemove.Add(Task);
			}
		}

		for (ULuxActionTask* Task : TasksToRemove)
		{
			if (Task)
			{
				Task->EndTask(true);
				Task->MarkAsGarbage();
			}
		}
	}

	// 구독한 모든 델리게이트를 해제합니다.
	if (UActionSystemComponent* ASC = GetActionSystemComponent())
	{
		for (const FActivePhaseTransition& ActiveTransition : ActivePhaseTransitions)
		{
			if (ActiveTransition.TransitionType == EPhaseTransitionType::OnGameplayEvent)
			{
				ASC->UnsubscribeFromGameplayEvent(ActiveTransition.EventTag, ActiveTransition.GameplayEventDelegate);
			}
			else if (ActiveTransition.TransitionType == EPhaseTransitionType::OnDurationEnd)
			{
				UnsubscribeFromTaskEvent(ActiveTransition.EventTag, ActiveTransition.TaskEventDelegate);
			}
			else if (ActiveTransition.TransitionType == EPhaseTransitionType::OnTaskEvent)
			{
				UnsubscribeFromTaskEvent(ActiveTransition.EventTag, ActiveTransition.TaskEventDelegate);
			}
		}
	}

	// 모든 전환 정보를 정리합니다.
	ActivePhaseTransitions.Empty();
}

void ULuxAction::ExecuteBehaviors(ULuxAction* Action, const TArray<FInstancedStruct>& Behaviors)
{
	UActionSystemComponent* ASC = Action->GetActionSystemComponent();
	AActor* Avatar = Action->GetAvatarActor();
	if (!ASC || !Avatar)
	{
		return;
	}

	ENetRole CurrentRole = Avatar->GetLocalRole();

	for (const FInstancedStruct& BehaviorStruct : Behaviors)
	{
		const FPhaseBehaviorBase& Behavior = BehaviorStruct.Get<const FPhaseBehaviorBase>();

		// 네트워크 실햄 정책 확인
		bool bShouldExecute = false;
		switch (Behavior.NetExecutionPolicy)
		{
		case EPhaseBehaviorNetExecutionPolicy::ServerOnly:
			bShouldExecute = (CurrentRole == ENetRole::ROLE_Authority);
			break;
		case EPhaseBehaviorNetExecutionPolicy::ClientOnly:
			bShouldExecute = (CurrentRole == ENetRole::ROLE_AutonomousProxy);
			break;
		case EPhaseBehaviorNetExecutionPolicy::All:
			bShouldExecute = true;
			break;
		}

		if (bShouldExecute)
		{
			Behavior.Execute(Action);
		}
	}
}

void ULuxAction::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
	// 자식 클래스에서 재정의
}

void ULuxAction::OnPhaseExit(const FGameplayTag& PhaseTag)
{
	// 자식 클래스에서 재정의
}

void ULuxAction::OnRep_ReplicatedPhaseInfo()
{
	// 서버 카운터가 로컬 카운터보다 클 때만 교정합니다. (클라이언트가 서버보다 느릴 때)
	if (ReplicatedPhaseInfo.PhaseHistoryCounter > LocalPhaseHistoryCounter)
	{
		LocalPhaseHistoryCounter = ReplicatedPhaseInfo.PhaseHistoryCounter;
		EnterPhase(ReplicatedPhaseInfo.PhaseTag, true);
	}
}

void ULuxAction::HandlePhaseTransitionEvent(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	// 페이즈 전환 중복 실행 방지
	if (bIsTransitioningPhase)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] 페이즈 전환 중 페이즈 전환 이벤트가 발생했습니다. 이벤트 태그: %s"), *GetLogPrefix(), *EventTag.ToString());
		return;
	}

	// 통합된 전환 정보에서 해당 이벤트에 맞는 전환을 찾습니다.
	for (const FActivePhaseTransition& ActiveTransition : ActivePhaseTransitions)
	{
		if (ActiveTransition.EventTag != EventTag)
		{
			continue;
		}

		// 조건 검사 (조건이 없으면 항상 통과)
		if (ActiveTransition.Transition.Conditions.Num() == 0 ||
			CheckAllConditions(*this, ActiveTransition.Transition.Conditions, Payload))
		{
			UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s] 페이즈 전환 조건 검사 통과. 다음 페이즈: %s"), *GetLogPrefix(), *ActiveTransition.Transition.NextPhaseTag.ToString());
			EnterPhase(ActiveTransition.Transition.NextPhaseTag, false);
			return;
		}
		else
		{
			UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s] 페이즈 전환 조건 검사 실패. 다음 페이즈: %s"), *GetLogPrefix(), *ActiveTransition.Transition.NextPhaseTag.ToString());
		}
	}
}

void ULuxAction::HandleGameplayEventForInterruption(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	if (EventTag != LuxGameplayTags::Event_Movement_Started)
	{
		return;
	}

	// 활성화된 몽타주 태스크를 찾아 중단시킵니다.
	for (ULuxActionTask* ActiveTask : ActiveTasks)
	{
		if (ULuxActionTask_PlayMontageAndWait* MontageTask = Cast<ULuxActionTask_PlayMontageAndWait>(ActiveTask))
		{
			MontageTask->EndTask(false);
			break;
		}
	}
}

bool ULuxAction::CheckAllConditions(const ULuxAction& Action, const TArray<FInstancedStruct>& Conditions, const FContextPayload& Payload)
{
	for (const FInstancedStruct& ConditionStruct : Conditions)
	{
		if (const FPhaseConditionBase* Condition = ConditionStruct.GetPtr<FPhaseConditionBase>())
		{
			if (!Condition->CheckCondition(Action, Payload))
			{
				// 단 하나의 조건이라도 실패하면 즉시 false를 반환합니다.
				return false;
			}
		}
	}

	return true;
}

/* ======================================== 태스크 및 이벤트 처리 (Task & Events) ======================================== */

void ULuxAction::PostTaskEvent(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	if (LifecycleState != ELuxActionLifecycleState::Executing)
	{
		return;
	}

	APawn* Avatar = Cast<APawn>(GetAvatarActor());
	if (!Avatar)
	{
		return;
	}

	// 클라이언트라면 서버로 이벤트 전송만 하고 로컬에서는 처리하지 않습니다.
	if (ReplicatedEventTags.HasTag(EventTag) && Avatar->IsLocallyControlled())
	{
		NotifyTaskEventToServer(EventTag, Payload);
		return;
	}

	// 스레드 안정성을 위해 락을 해제한 후 델리게이트를 실행합니다.
	{
		FScopeLock Lock(&EventHandlersCS);

		if (FLuxTaskEvent* Handler = EventHandlers.Find(EventTag))
		{
			FLuxTaskEvent HandlerCopy = *Handler;
			Lock.Unlock();

			HandlerCopy.Broadcast(EventTag, Payload);
		}
	}
}

void ULuxAction::SubscribeToTaskEvent(const FGameplayTag& EventTag, const FScriptDelegate& InDelegate)
{
	if (!InDelegate.IsBound())
	{
		return;
	}

	if (LifecycleState != ELuxActionLifecycleState::Executing)
	{
		return;
	}

	FScopeLock Lock(&EventHandlersCS);

	// 기존 델리게이트를 제거한 후 새로 등록하여 중복을 방지합니다.
	FLuxTaskEvent& Event = EventHandlers.FindOrAdd(EventTag);
	if (!Event.Contains(InDelegate))
	{
		Event.Add(InDelegate);
	}
}

void ULuxAction::UnsubscribeFromTaskEvent(const FGameplayTag& EventTag, const FScriptDelegate& InDelegate)
{
	if (!InDelegate.IsBound())
	{
		return;
	}

	if (LifecycleState != ELuxActionLifecycleState::Executing)
	{
		return;
	}

	FScopeLock Lock(&EventHandlersCS);
	if (FLuxTaskEvent* Event = EventHandlers.Find(EventTag))
	{
		Event->Remove(InDelegate);
	}
}

void ULuxAction::OnTaskActivated(ULuxActionTask* Task)
{
	if (Task)
	{
		ActiveTasks.Add(Task);
	}
}

void ULuxAction::OnTaskEnded(ULuxActionTask* Task)
{
	if (Task)
	{
		ActiveTasks.Remove(Task);
		Task->MarkAsGarbage();
	}
}

void ULuxAction::OnTaskRehomed(ULuxActionTask* Task)
{

}

void ULuxAction::NotifyTaskEventToServer(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC) return;

	ASC->Server_ReceiveTaskEvent(GetActiveHandle(), EventTag, Payload);
}

void ULuxAction::AddTaskResultStoreRequest(const FGameplayTag& EventTag, FName SourceKey, FName DestinationKey)
{
	FTaskResultStoreRequest NewRequest = { SourceKey, DestinationKey };
	TaskResultStoreRequests.FindOrAdd(EventTag).Add(NewRequest);

	// 이 이벤트 태그에 대한 핸들러가 아직 등록되지 않았다면 범용 핸들러를 바인딩합니다.
	if (!EventHandlers.Contains(EventTag))
	{
		FScriptDelegate DelegateToBind;
		DelegateToBind.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxAction, HandleTaskResultStoringEvent));
		SubscribeToTaskEvent(EventTag, DelegateToBind);
	}
}

void ULuxAction::HandleTaskResultStoringEvent(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	if (!ActionPayload.IsValid())
	{
		return;
	}

	// 이벤트 태그에 해당하는 모든 저장 요청을 처리합니다.
	const TArray<FTaskResultStoreRequest>* Requests = TaskResultStoreRequests.Find(EventTag);
	if (!Requests)
	{
		return;
	}

	for (const FTaskResultStoreRequest& Request : *Requests)
	{
		if (const FInstancedStruct* SourceData = Payload.GetData<FInstancedStruct>(Request.SourceKey))
		{
			ActionPayload->DataArray.Add(*SourceData);
			ActionPayload->DataKeys.Add(Request.DestinationKey);
		}
	}
}

void ULuxAction::AddSpawnedActor(AActor* Actor)
{
	if (Actor) SpawnedActors.Add(Actor);
}

/* ======================================== 태그 처리 (Tag) ======================================== */

void ULuxAction::AddTag(const FGameplayTag& Tag, int32 StackCount)
{
	if (!Tag.IsValid() || StackCount <= 0) return;

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (ASC)
	{
		ASC->AddTag(Tag, StackCount);
		GrantedTags.AddStack(Tag, StackCount);
	}
}

void ULuxAction::RemoveTag(const FGameplayTag& Tag, int32 StackCount)
{
	if (!Tag.IsValid() || StackCount <= 0) return;

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (ASC)
	{
		// 액션이 해당 태그를 부여한 기록이 있는지(스택이 1 이상인지) 확인합니다.
		if (GrantedTags.GetStackCount(Tag) > 0)
		{
			ASC->RemoveTag(Tag, 1);
			GrantedTags.RemoveStack(Tag, 1);
		}
	}
}

void ULuxAction::AddTags(const FGameplayTagContainer& TagsToAdd, int32 StackCount)
{
	if (TagsToAdd.IsEmpty() || StackCount <= 0) return;

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC) return;

	for (const FGameplayTag& SingleTag : TagsToAdd)
	{
		ASC->AddTag(SingleTag, 1);
		GrantedTags.AddStack(SingleTag, StackCount);
	}
}

void ULuxAction::RemoveTags(const FGameplayTagContainer& TagsToRemove, int32 StackCount)
{
	if (TagsToRemove.IsEmpty() || StackCount <= 0) return;

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC) return;

	for (const FGameplayTag& SingleTag : TagsToRemove)
	{
		// 컨테이너의 각 태그에 대해 스택 카운트를 개별적으로 확인합니다.
		if (GrantedTags.GetStackCount(SingleTag) > 0)
		{
			ASC->RemoveTag(SingleTag, 1);
			GrantedTags.RemoveStack(SingleTag, 1);
		}
	}
}

void ULuxAction::TransferStateFrom(ULuxAction* PredictedAction)
{
	if (!PredictedAction)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s] TransferStateFrom: PredictedAction이 null입니다."), *GetLogPrefix());
		return;
	}

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 예측 액션 [%s]에서 상태를 이전합니다."), *GetLogPrefix(), *PredictedAction->GetName());

	// 1. 태스크 소유권 이전
	if (PredictedAction->ActiveTasks.Num() > 0)
	{
		ActiveTasks.Empty(); // 기존 태스크 목록 초기화
		
		TArray<TObjectPtr<ULuxActionTask>>& PredictedTasks = PredictedAction->ActiveTasks;
		for (int32 i = PredictedTasks.Num() - 1; i >= 0; --i)
		{
			ULuxActionTask* PredictedTask = PredictedTasks[i];
			if (PredictedTask)
			{
				UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 태스크 '%s'의 소유권을 이전합니다."), *GetLogPrefix(), *GetNameSafe(PredictedTask));
				PredictedTask->ReHome(this);
				ActiveTasks.Add(PredictedTask);
			}
		}
		
		// 예측 액션의 태스크 목록 정리
		PredictedAction->ActiveTasks.Empty();
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 태스크 %d개를 이전했습니다."), *GetLogPrefix(), ActiveTasks.Num());
	}

	// 2. 카메라 모드 상태 이전
	if (PredictedAction->PushedCameraModes.Num() > 0)
	{
		PushedCameraModes = MoveTemp(PredictedAction->PushedCameraModes);
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 카메라 모드 %d개를 이전했습니다."), *GetLogPrefix(), PushedCameraModes.Num());
	}

	// 3. 부여된 태그 상태 이전
	if (PredictedAction->GrantedTags.GetExplicitGameplayTags().Num() > 0)
	{
		GrantedTags = PredictedAction->GrantedTags;
		PredictedAction->GrantedTags.Reset();
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 부여된 태그들을 이전했습니다."), *GetLogPrefix());
	}

	// 4. 스폰된 액터 상태 이전
	if (PredictedAction->SpawnedActors.Num() > 0)
	{
		SpawnedActors.Append(PredictedAction->SpawnedActors);
		PredictedAction->SpawnedActors.Empty();
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 스폰된 액터 %d개를 이전했습니다."), *GetLogPrefix(), SpawnedActors.Num());
	}

	// 5. 액션 페이로드 이전 (필요시)
	if (PredictedAction->ActionPayload.IsValid())
	{
		ActionPayload = PredictedAction->ActionPayload;
		PredictedAction->ActionPayload.Reset();
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 액션 페이로드를 이전했습니다."), *GetLogPrefix());
	}

	// 6. 페이즈 히스토리 카운터 이전
	LocalPhaseHistoryCounter = PredictedAction->LocalPhaseHistoryCounter;

	// 7. 현재 페이즈 정보 이전
	CurrentPhaseTag = PredictedAction->CurrentPhaseTag;

	// 8. 예측 액션에 ReHome 플래그 설정하여 정리 로직에서 중복 처리를 방지
	PredictedAction->SetBeingReHomed(true);

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] TransferStateFrom: 상태 이전이 완료되었습니다."), *GetLogPrefix());
}

FString ULuxAction::GetLogPrefix() const
{
	if (ClientServerString.IsEmpty())
	{
		ClientServerString = GetClientServerContextString(OwningActorInfo.AvatarActor.Get());
	}

	return FString::Printf(TEXT("%s Action [%s]: "), *ClientServerString, *GetName());
}
