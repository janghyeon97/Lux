// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/ActionSystemComponent.h"

// 코어 액션 시스템
#include "ActionSystem/ActionSystemGlobals.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Actions/LuxActionCost.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystem/Actions/Phase/LuxActionPhaseData.h"
#include "ActionSystem/Executions/LuxExecutionCalculation.h"
#include "ActionSystem/LuxActionTagRelationshipMapping.h"
#include "Attributes/LuxAttributeSet.h"
#include "Cues/LuxCueManager.h"
#include "Effects/LuxEffect.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "Tasks/LuxActionTask.h"
#include "ActionSystem/Cooldown/LuxCooldownTracker.h"

// 게임 프레임워크
#include "Game/LuxPlayerController.h"
#include "Game/LuxPlayerState.h"

// 언리얼 엔진 헤더
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"


UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_ActionInputBlocked, "Gameplay.ActionInputBlocked");

UActionSystemComponent::UActionSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;

	LuxActionSpecs.OwnerComponent = this;
	ActiveLuxActions.OwnerComponent = this;
	ActiveLuxEffects.OwnerComponent = this;
	GrantedTags.OwnerComponent = this;

	// 쿨다운 트래커는 런타임에 생성합니다. (CDO/아키타입 금지)
	CooldownTracker = nullptr;
}

void UActionSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, OwnerActor);
	DOREPLIFETIME(ThisClass, AvatarActor);
	DOREPLIFETIME(ThisClass, GrantedTags);

	DOREPLIFETIME(ThisClass, LuxActionSpecs);
	DOREPLIFETIME(ThisClass, ActiveLuxActions);
	DOREPLIFETIME(ThisClass, ActiveLuxEffects);
	DOREPLIFETIME(ThisClass, GreantedAttributes);
	DOREPLIFETIME(ThisClass, CooldownTracker);
}

void UActionSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 컴포넌트에 입력이 들어온 것이 있는지 매 프레임 확인합니다.
	if (InputPressedSpecHandles.Num() > 0 || InputReleasedSpecHandles.Num() > 0 || InputHeldSpecHandles.Num() > 0)
	{
		ProcessActionInput(DeltaTime, false);
	}

	ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActor());
	if (AvatarCharacter && AvatarCharacter->GetCharacterMovement())
	{
		FContextPayload Payload;
		FPayload_GameplayEventData EventData;
		EventData.Instigator = GetAvatarActor();
		Payload.SetData(LuxPayloadKeys::GameplayEventData, EventData);

		const bool bNowMoving = AvatarCharacter->GetCharacterMovement()->GetCurrentAcceleration().SizeSquared() > 0.f;
		if (bNowMoving)
		{
			BroadcastGameplayEventToSubscribers(LuxGameplayTags::Event_Movement_Started, Payload);
		}

		// 움직임 상태가 이전 프레임과 달라졌을 때만 이벤트를 발생시킵니다.
		if (bNowMoving != bIsMoving)
		{
			bIsMoving = bNowMoving;
			if (!bIsMoving)
			{
				BroadcastGameplayEventToSubscribers(LuxGameplayTags::Event_Movement_Stopped, Payload);
			}
		}
	}

	if (PendingKillActions.Num() > 0)
	{
		TArray<TObjectPtr<ULuxAction>> ActionsToKill = PendingKillActions;
		PendingKillActions.Empty();

		for (ULuxAction* Action : ActionsToKill)
		{
			if (Action && !Action->IsGarbageEliminationEnabled())
			{
				Action->MarkAsGarbage();
			}
		}
	}
}

void UActionSystemComponent::OnRegister()
{
	Super::OnRegister();


}

void UActionSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UActionSystemGlobals::IsValid())
	{
		UActionSystemGlobals::Get().RegisterComponent(this);
	}

	IsUsingRegisteredSubObjectList();

	// 서버와 클라이언트 모두에서 쿨다운 트래커 인스턴스를 생성
	if (!CooldownTracker)
	{
		CreateCooldownTracker();
	}
}

void UActionSystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UActionSystemGlobals::IsValid())
	{
		UActionSystemGlobals::Get().UnregisterComponent(this);
	}

	if (GetOwnerRole() == ENetRole::ROLE_Authority)
	{
		RemoveAllActions();
		RemoveAllActiveEffects();
	}

	if (PendingKillActions.Num() > 0)
	{
		for (int32 i = PendingKillActions.Num() - 1; i >= 0; --i)
		{
			PendingKillActions[i]->MarkAsGarbage();
		}
	}

	// 클라이언트의 보류 중인 예측 액션을 정리합니다.
	for (auto const& [Key, PredictedAction] : PendingPredictions)
	{
		if (PredictedAction)
		{
			PredictedAction->MarkAsGarbage();
		}
	}
	PendingPredictions.Empty();

	// 쿨다운 트래커 등록 해제 및 정리
	if (CooldownTracker)
	{
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(CooldownTracker);
		}
		CooldownTracker->MarkAsGarbage();
		CooldownTracker = nullptr;
	}

	ClearActorInfo();

	Super::EndPlay(EndPlayReason);
}

/* ======================================== ActorInfo Management ======================================== */

void UActionSystemComponent::InitActorInfo(AActor* NewOwnerActor, AActor* NewAvatarActor)
{
	OwnerActor = NewOwnerActor;
	AvatarActor = NewAvatarActor;

	if (!OwnerActor.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("UActionSystemComponent::InitActorInfo - OwnerActor is null!"));
		return;
	}

	TArray<UObject*> SubObjects;
	GetObjectsWithOuter(OwnerActor.Get(), SubObjects, true);

	for (UObject* SubObject : SubObjects)
	{
		ULuxAttributeSet* FoundAttribute = Cast<ULuxAttributeSet>(SubObject);
		if (FoundAttribute && GetAttributeSet(FoundAttribute->GetClass()) == nullptr)
		{
			AddAttributeSet(FoundAttribute);
		}
	}
}

void UActionSystemComponent::SetOwnerActor(AActor* NewOwnerActor)
{
	InitActorInfo(NewOwnerActor, AvatarActor.Get());
}

void UActionSystemComponent::SetAvatarActor(AActor* NewAvatarActor)
{
	InitActorInfo(OwnerActor.Get(), NewAvatarActor);
}

void UActionSystemComponent::ClearActorInfo()
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	RemoveAllAttributes();

	OwnerActor = nullptr;
	AvatarActor = nullptr;

	UE_LOG(LogLuxActionSystem, Log, TEXT("ActorInfo and all associated data cleared."));
}

/* ======================================== Action Management ======================================== */

FLuxActionSpecHandle UActionSystemComponent::K2_GrantAction(TSubclassOf<ULuxAction> ActionClass, FGameplayTag& InputTag, int Level)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority())
	{
		return FLuxActionSpecHandle();
	}

	if (!ActionClass)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("GrantAction failed: ActionClass is null."));
		return FLuxActionSpecHandle();
	}

	const ULuxAction* ActionCDO = GetDefault<ULuxAction>(ActionClass);
	if (!ActionCDO)
	{
		return FLuxActionSpecHandle();
	}

	// InstancedPerActor 정책인 경우, 이미 부여되었는지 확인합니다.
	if (ActionCDO->InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerActor)
	{
		FRWScopeLock ReadLock(ActionSpecsLock, FRWScopeLockType::SLT_ReadOnly);
		for (const FLuxActionSpec& ExistingSpec : LuxActionSpecs.Items)
		{
			if (ExistingSpec.Action && ExistingSpec.Action->GetClass() == ActionClass)
			{
				UE_LOG(LogLuxActionSystem, Warning, TEXT("GrantAction failed for '%s': Action with InstancedPerActor policy is already granted."), *ActionClass->GetName());
				return ExistingSpec.Handle; // 이미 존재하므로 기존 핸들을 반환
			}
		}
	}

	FLuxActionSpecHandle NewHandle;
	bool bShouldActivateOnGrant = false;

	// === 쓰기 락(Write Lock) 범위 시작 ===
	{
		FRWScopeLock WriteLock(ActionSpecsLock, FRWScopeLockType::SLT_Write);
		FLuxActionSpec& NewSpec = LuxActionSpecs.Items.Add_GetRef(FLuxActionSpec(ActionClass, InputTag, Level));
		//ActionSpecMap.Add(NewSpec.Handle, &NewSpec);
		LuxActionSpecs.MarkArrayDirty();

		if (ActionCDO->InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerActor)
		{
			NewSpec.Action = NewObject<ULuxAction>(this, ActionClass);
			AddReplicatedSubObject(NewSpec.Action);
		}
		else
		{
			NewSpec.Action = const_cast<ULuxAction*>(ActionCDO);
		}

		if (NewSpec.Action)
		{
			for (const FGameplayTag& TriggerTag : NewSpec.Action->EventTriggerTags)
			{
				EventTriggerMap.FindOrAdd(TriggerTag).Add(NewSpec.Handle);
			}

			if (NewSpec.Action->ActivationPolicy == ELuxActionActivationPolicy::OnGrant ||
				NewSpec.Action->ActivationPolicy == ELuxActionActivationPolicy::OnGrantAndRemove)
			{
				bShouldActivateOnGrant = true;
			}
		}

		LuxActionSpecs.MarkItemDirty(NewSpec);
		NewHandle = NewSpec.Handle;
	}
	// === 쓰기 락(Write Lock) 범위 종료 ===

	if (bShouldActivateOnGrant)
	{
		TryExecuteAction(NewHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("Granted Action '%s' with Handle ID %d"), *ActionClass->GetName(), NewHandle.Handle);
	return NewHandle;
}

FLuxActionSpecHandle UActionSystemComponent::GrantAction(FLuxActionSpec& InSpec)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority())
	{
		return FLuxActionSpecHandle();
	}

	if (!InSpec.Action)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("GrantAction failed: Action is null."));
		return FLuxActionSpecHandle();
	}

	const ULuxAction* ActionCDO = InSpec.Action->GetClass()->GetDefaultObject<ULuxAction>();
	if (!ActionCDO)
	{
		return FLuxActionSpecHandle();
	}

	// InstancedPerActor 정책인 경우, 이미 부여되었는지 확인합니다.
	if (InSpec.Action->InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerActor)
	{
		FRWScopeLock ReadLock(ActionSpecsLock, FRWScopeLockType::SLT_ReadOnly);
		for (const FLuxActionSpec& ExistingSpec : LuxActionSpecs.Items)
		{
			if (ExistingSpec.Action && ExistingSpec.Action->GetClass() == InSpec.Action->GetClass())
			{
				UE_LOG(LogLuxActionSystem, Warning, TEXT("GrantAction failed for '%s': Action with InstancedPerActor policy is already granted."), *InSpec.Action->GetName());
				return ExistingSpec.Handle; // 이미 존재하므로 기존 핸들을 반환
			}
		}
	}

	FLuxActionSpecHandle NewHandle;
	bool bShouldActivateOnGrant = false;

	{
		FRWScopeLock WriteLock(ActionSpecsLock, FRWScopeLockType::SLT_Write);

		// 완성된 Spec을 컨테이너에 추가합니다.
		FLuxActionSpec& NewSpec = LuxActionSpecs.Items.Add_GetRef(InSpec);
		//ActionSpecMap.Add(NewSpec.Handle, &NewSpec);
		LuxActionSpecs.MarkArrayDirty();

		if (ActionCDO->InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerActor)
		{
			// 정책이 'InstancedPerActor' 이면 인스턴스를 생성하여 Spec에 저장합니다.z`
			NewSpec.Action = NewObject<ULuxAction>(this, ActionCDO->GetClass());
			AddReplicatedSubObject(NewSpec.Action);
		}

		if (NewSpec.Action)
		{
			// 어빌리티의 이벤트 트리거 태그를 이벤트 트리거 맵에 추가합니다.
			for (const FGameplayTag& TriggerTag : NewSpec.Action->EventTriggerTags)
			{
				EventTriggerMap.FindOrAdd(TriggerTag).Add(NewSpec.Handle);
			}

			// 어빌리티의 활성화 정책이 'OnGrant' 이면 어빌리티를 활성화 시도합니다.
			if (NewSpec.Action->ActivationPolicy == ELuxActionActivationPolicy::OnGrant || 
				NewSpec.Action->ActivationPolicy == ELuxActionActivationPolicy::OnGrantAndRemove)
			{
				bShouldActivateOnGrant = true;
			}
		}

		LuxActionSpecs.MarkItemDirty(NewSpec);
		NewHandle = NewSpec.Handle;
	}

	// 락이 해제된 후에 안전하게 액션 활성화를 시도합니다.
	if (bShouldActivateOnGrant)
	{
		TryExecuteAction(NewHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("Granted Action '%s' with Handle ID %d"), *InSpec.Action->GetName(), NewHandle.Handle);
	return NewHandle;
}

void UActionSystemComponent::RemoveAction(FLuxActionSpecHandle Handle)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	// 배열에서 제거할 Spec의 인덱스를 직접 찾습니다.
	const int32 SpecIndex = LuxActionSpecs.Items.IndexOfByPredicate([&Handle](const FLuxActionSpec& Spec) {
		return Spec.Handle == Handle;
		});

	// 제거할 액션이 없으면 여기서 안전하게 종료됩니다.
	if (SpecIndex == INDEX_NONE)
	{
		return;
	}

	const FLuxActionSpec SpecToRemove = LuxActionSpecs.Items[SpecIndex];

	// 액션이 현재 실행 중이라면 종료시킵니다.
	if (SpecToRemove.IsActive())
	{
		FActiveLuxAction* ActiveAction = ActiveLuxActions.Items.FindByPredicate([&Handle](const FActiveLuxAction& Action) {
			return Action.Spec.Handle == Handle;
			});

		if (ActiveAction)
		{
			if (ActiveAction->Action)
			{
				ActiveAction->Action->OnActionEnd(true); // 액션 자체의 정리 로직 호출 (취소됨)
			}

			ActiveLuxActions.Items.Remove(*ActiveAction);
			ActiveLuxActions.MarkArrayDirty();
		}
	}

	ULuxAction* ActionObject = SpecToRemove.Action;
	if (ActionObject)
	{
		// 이벤트 트리거 맵 정리
		for (const FGameplayTag& TriggerTag : ActionObject->EventTriggerTags)
		{
			if (TArray<FLuxActionSpecHandle>* Handles = EventTriggerMap.Find(TriggerTag))
			{
				Handles->Remove(Handle);
			}
		}

		// 인스턴스화된 액터라면 소멸 처리
		if (ActionObject->GetInstancingPolicy() == ELuxActionInstancingPolicy::InstancedPerActor)
		{
			RemoveReplicatedSubObject(ActionObject);
			ActionObject->MarkAsGarbage();
		}
	}

	// 입력 핸들 정리
	{
		FRWScopeLock InputWriteLock(InputHandlesLock, FRWScopeLockType::SLT_Write);
		InputPressedSpecHandles.Remove(Handle);
		InputReleasedSpecHandles.Remove(Handle);
		InputHeldSpecHandles.Remove(Handle);
	}

	// 마지막으로 메인 배열에서 Spec을 제거합니다.
	LuxActionSpecs.Items.RemoveAt(SpecIndex);
	LuxActionSpecs.MarkArrayDirty();
}

void UActionSystemComponent::RemoveAllActions()
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	// 현재 실행 중인 모든 액션을 종료시킵니다.
	if (ActiveLuxActions.Items.Num() > 0)
	{
		TArray<FActiveLuxAction> ActiveActionsToCancel = ActiveLuxActions.Items;
		for (const FActiveLuxAction& ActionInfo : ActiveActionsToCancel)
		{
			if (ULuxAction* ActionInstance = ActionInfo.Action.Get())
			{
				ActionInstance->CancelAction();
			}
		}
	}

	// 부여된 모든 액션 Spec과 관련 데이터를 제거합니다.
	{
		FRWScopeLock ActionSpecsWriteLock(ActionSpecsLock, FRWScopeLockType::SLT_Write);

		for (int32 i = LuxActionSpecs.Items.Num() - 1; i >= 0; --i)
		{
			const FLuxActionSpec& Spec = LuxActionSpecs.Items[i];
			if (ULuxAction* ActionObject = Spec.Action)
			{
				// 이벤트 트리거 맵에서 핸들 제거
				for (const FGameplayTag& TriggerTag : ActionObject->EventTriggerTags)
				{
					if (TArray<FLuxActionSpecHandle>* Handles = EventTriggerMap.Find(TriggerTag))
					{
						Handles->Remove(Spec.Handle);
					}
				}

				// InstancedPerActor 정책으로 생성된 액션 인스턴스 소멸 처리
				if (ActionObject->GetInstancingPolicy() == ELuxActionInstancingPolicy::InstancedPerActor)
				{
					RemoveReplicatedSubObject(ActionObject);
					ActionObject->OnActionEnd(true);
					PendingKillActions.Add(ActionObject);
				}
			}
		}

		LuxActionSpecs.Items.Empty();
		LuxActionSpecs.MarkArrayDirty();

		// 이 Spec들을 참조하던 입력 핸들도 비웁니다.
		{
			FRWScopeLock InputWriteLock(InputHandlesLock, FRWScopeLockType::SLT_Write);
			InputPressedSpecHandles.Empty();
			InputReleasedSpecHandles.Empty();
			InputHeldSpecHandles.Empty();
		}
	}
}

// ======================================== Action Activation & Execution ========================================

void UActionSystemComponent::TryExecuteAction(FLuxActionSpecHandle Handle)
{
	// 실행하려는 액션의 Spec을 찾습니다.
	FLuxActionSpec* FoundSpec = FindActionSpecFromHandle(Handle);
	if (!FoundSpec || !FoundSpec->Action)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("TryExecuteAction failed: Action spec not found or Action is null for Handle %d."), Handle.Handle);
		return;
	}

	// 서버가 아니고 액션이 이미 서버의 응답을 기다리는 중이라면 중복 실행을 막습니다.
	if (PendingPredictedActions.Contains(Handle))
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("TryExecuteAction failed: Action for Handle %d is already pending server confirmation."), Handle.Handle);
		return;
	}

	// 서버와 클라이언트 모두 액션 실행이 가능한지 확인합니다. (예: 쿨다운, 자원 등)
	FGameplayTagContainer FailureTags;
	if (!CanActivateAction(Handle, FailureTags))
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("CanActivateAction failed for %s. Reason: %s"), *FoundSpec->Action->GetName(), *FailureTags.ToString());
		NotifyActionFailed(Handle, FailureTags);
		return;
	}

	// 네트워크 권한에 따라 로직을 분기합니다.
	if (GetOwner()->HasAuthority())
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[Server] TryExecuteAction called directly. Executing implementation for '%s'."), *FoundSpec->Action->GetName());
		Server_TryExecuteAction(Handle, FLuxPredictionKey());
		return;
	}

	PendingPredictedActions.Add(Handle);

	FOwningActorInfo ActorInfo;
	ActorInfo.ActionSystemComponent = this;
	ActorInfo.OwnerActor = OwnerActor.IsValid() ? OwnerActor : nullptr;
	ActorInfo.AvatarActor = AvatarActor.IsValid() ? AvatarActor : nullptr;

	APawn* OwnerPawn = OwnerActor.IsValid() ? Cast<APawn>(OwnerActor) : nullptr;
	ActorInfo.Controller = Cast<APawn>(OwnerActor) ? OwnerPawn->GetController() : nullptr;

	ULuxAction* PredictedInstance = nullptr;
	if (FoundSpec->Action->GetInstancingPolicy() == ELuxActionInstancingPolicy::InstancedPerExecution)
	{
		FName PredictedInstanceName = FName(*FString::Printf(TEXT("Predicted_%s"), *FoundSpec->Action->GetClass()->GetName()));
		UObject* ActionCDO = FoundSpec->Action->GetClass()->GetDefaultObject();
		PredictedInstance = NewObject<ULuxAction>(this, ActionCDO->GetClass(), PredictedInstanceName);
	}
	else
	{
		PredictedInstance = FoundSpec->Action;
	}

	// 예측 키를 생성하고, 예측 목록에 추가합니다.
	const FLuxPredictionKey PredictionKey = CreatePredictionKey();
	PendingPredictions.Add(PredictionKey, PredictedInstance);

	// 임시 ActiveAction을 생성하여 클라이언트에서 Action을 실행합니다.
	FActiveLuxAction TempActiveAction(*FoundSpec, PredictionKey, ActorInfo);
	TempActiveAction.StartTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogLuxActionSystem, Error, TEXT("================>>> [CLIENT] Executing Action: %s | Owner: %s | InstancingPolicy: %s | PredictionKey: %d ---"),
		*PredictedInstance->GetName(),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(PredictedInstance->GetInstancingPolicy()),
		PredictionKey.Key
	);

	// 서버에 Handle과 예측 키를 담아 실행을 요청합니다.
	Server_TryExecuteAction(Handle, PredictionKey);

	PredictedInstance->ExecuteAction(ActorInfo, *FoundSpec, TempActiveAction.Handle);
}

void UActionSystemComponent::Server_TryExecuteAction_Implementation(const FLuxActionSpecHandle Handle, FLuxPredictionKey PredictionKey)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	FGameplayTagContainer FailureTags;
	FLuxActionSpec* FoundSpec = FindActionSpecFromHandle(Handle);
	if (!FoundSpec || !::IsValid(FoundSpec->Action))
	{
		NotifyActionFailed(FoundSpec->Handle, FailureTags);
		Client_ConfirmAction(PredictionKey, false);
		return;
	}

	const ULuxAction* ActionToExecute = FoundSpec->Action;


	if (!CanActivateAction(Handle, FailureTags))
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("SERVER REJECTED activation for '%s'. Reason: %s"), *FoundSpec->Action->GetName(), *FailureTags.ToString());
		NotifyActionFailed(FoundSpec->Handle, FailureTags);
		Client_ConfirmAction(PredictionKey, false);
		return;
	}

	// --- 핵심 로직: 액션 고유 태그로 즉시 잠금 ---
	if (FoundSpec->Action->ActionIdentifierTag.IsValid())
	{
		AddTag(FoundSpec->Action->ActionIdentifierTag, 1);
	}

	// 새로운 액션이 시작될 때 이전 몽타주를 강제로 중단
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (Character)
	{
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			// 현재 재생 중인 모든 몽타주를 즉시 중단
			AnimInstance->StopAllMontages(0.0f);
			UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] 새로운 액션 시작: 이전 몽타주들을 강제로 중단했습니다."), *FoundSpec->Action->GetName());
		}
	}

	FOwningActorInfo ActorInfo;
	ActorInfo.ActionSystemComponent = this;
	ActorInfo.OwnerActor = OwnerActor.IsValid() ? OwnerActor : nullptr;
	ActorInfo.AvatarActor = AvatarActor.IsValid() ? AvatarActor : nullptr;
	APawn* OwnerPawn = OwnerActor.IsValid() ? Cast<APawn>(OwnerActor) : nullptr;
	ActorInfo.Controller = Cast<APawn>(OwnerActor) ? OwnerPawn->GetController() : nullptr;

	FActiveLuxAction& NewActiveAction = ActiveLuxActions.Items.Emplace_GetRef(FActiveLuxAction(*FoundSpec, PredictionKey, ActorInfo));
	if (ActionToExecute->GetInstancingPolicy() == ELuxActionInstancingPolicy::InstancedPerExecution)
	{
		// 정책이 'InstancedPerExecution' 이면 인스턴스를 생성하여 ActiveLuxAction 에 저장합니다.
		NewActiveAction.Action = NewObject<ULuxAction>(this, ActionToExecute->GetClass());
		AddReplicatedSubObject(NewActiveAction.Action.Get());
	}
	else
	{
		NewActiveAction.Action = FoundSpec->Action;
	}

	UE_LOG(LogLuxActionSystem, Error, TEXT("================>>> [SERVER] Executing Action: %s | Owner: %s | InstancingPolicy: %s | PredictionKey: %d ---"),
		*NewActiveAction.Action->GetName(),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(ActionToExecute->GetInstancingPolicy()),
		PredictionKey.Key
	);

	//ActiveActionMap.Add(NewActiveAction.Handle, &NewActiveAction);

	// 이 액션이 취소시켜야 할 태그들을 가져옵니다.
	FGameplayTagContainer MappedCancelTags;
	MappedCancelTags.AppendTags(FoundSpec->Action->CancelActionsWithTag);

	if (TagRelationshipMapping)
	{
		TagRelationshipMapping->GetCancelledByTags(FoundSpec->Action->ActionTags, MappedCancelTags);
	}

	if (MappedCancelTags.Num() > 0)
	{
		CancelActions(&MappedCancelTags, nullptr);
	}

	FoundSpec->ActivationCount++;
	FoundSpec->DynamicTags.AppendTags(FoundSpec->Action->ActionTags);

	NewActiveAction.StartTime = GetWorld()->GetTimeSeconds();
	NewActiveAction.Action->ActivateAction(*FoundSpec, ActorInfo);
	NewActiveAction.Action->ExecuteAction(ActorInfo, *FoundSpec, NewActiveAction.Handle);

	FoundSpec->LastExecutionTime = NewActiveAction.StartTime;

	LuxActionSpecs.MarkItemDirty(*FoundSpec);
	ActiveLuxActions.MarkItemDirty(NewActiveAction);
	ActiveLuxActions.MarkArrayDirty();


	NotifyActionActivated(NewActiveAction.Handle);
	Client_ConfirmAction(PredictionKey, true);
}

void UActionSystemComponent::Client_ConfirmAction_Implementation(FLuxPredictionKey Key, bool bSuccess)
{
	// 서버로부터 응답을 받았으므로, 더 이상 'pending' 상태가 아닙니다.
	// 예측 목록에서 해당 액션을 찾아 잠금을 해제합니다.
	if (TObjectPtr<ULuxAction>* FoundPredictedInstancePtr = PendingPredictions.Find(Key))
	{
		if (ULuxAction* PredictedAction = *FoundPredictedInstancePtr)
		{
			if (const FLuxActionSpec* Spec = PredictedAction->GetLuxActionSpec())
			{
				PendingPredictedActions.Remove(Spec->Handle);
			}
		}

		// 실패했다면 예측을 롤백하고 예측 목록에서 제거합니다.
		if (!bSuccess && *FoundPredictedInstancePtr)
		{
			(*FoundPredictedInstancePtr)->CancelAction();
			PendingPredictions.Remove(Key);
		}
	}
}

void UActionSystemComponent::ReHomePredictedActionTasks(FActiveLuxAction& AuthoritativeAction)
{
	ULuxAction* AuthoritativeActionPtr = AuthoritativeAction.Action;
	if (!AuthoritativeActionPtr)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("태스크 소유권 이전 실패 (예측 키: %d): 서버로부터 받은 권위 있는 액션 인스턴스가 비어있습니다."), AuthoritativeAction.PredictionKey.Key);
		return;
	}

	AuthoritativeActionPtr->OwningActorInfo = AuthoritativeAction.ActorInfo;
	AuthoritativeActionPtr->ActiveActionHandle = AuthoritativeAction.Handle;

	// 인스턴싱 정책에 따라 분기합니다.
	const ELuxActionInstancingPolicy InstancingPolicy = AuthoritativeActionPtr->GetInstancingPolicy();
	if (InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerExecution)
	{
		ReHomeExecutionInstancedAction(AuthoritativeAction);
	}
	else if (InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerActor)
	{
		ReHomeActorInstancedAction(AuthoritativeAction);
	}

	// 서버로부터 복제된 현재 페이즈 이름을 기반으로 PhaseData를 가져옵니다.
	if (AuthoritativeActionPtr->ActionPhaseData && AuthoritativeActionPtr->ActionPhaseData->Phases.Contains(AuthoritativeActionPtr->CurrentPhaseTag))
	{
		const FActionPhaseData& CurrentPhaseData = AuthoritativeActionPtr->ActionPhaseData->Phases[AuthoritativeActionPtr->CurrentPhaseTag];
		AuthoritativeActionPtr->SetupPhaseTransitions(CurrentPhaseData, this);
	}

	PendingPredictions.Remove(AuthoritativeAction.PredictionKey);

	UE_LOG(LogLuxActionSystem, Warning, TEXT("--- [ReHome] 액션 [%s] (정책: %s)의 소유권 이전이 완료되었습니다 ---"), *AuthoritativeActionPtr->GetName(), *UEnum::GetValueAsString(InstancingPolicy));
}

void UActionSystemComponent::ReHomeExecutionInstancedAction(FActiveLuxAction& AuthoritativeAction)
{
	ULuxAction* AuthoritativeActionPtr = AuthoritativeAction.Action;

	// 예측용 임시 인스턴스를 찾아 태스크를 옮기고 파괴합니다.
	TObjectPtr<ULuxAction>* FoundPredictedInstancePtr = PendingPredictions.Find(AuthoritativeAction.PredictionKey);
	if (!FoundPredictedInstancePtr || !(*FoundPredictedInstancePtr))
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("ReHomeExecutionInstancedAction (%s, 예측 키: %d): 클라이언트의 예측 액션 인스턴스를 찾을 수 없습니다."), *AuthoritativeActionPtr->GetName(), AuthoritativeAction.PredictionKey.Key);
		return;
	}

	ULuxAction* PredictedAction = *FoundPredictedInstancePtr;
	LogReHomeDetails(AuthoritativeAction, PredictedAction);

	AuthoritativeActionPtr->LifecycleState = ELuxActionLifecycleState::Executing;

	// 예측 액션의 모든 상태를 서버에서 복제된 액션으로 이전합니다 (태스크, 카메라 모드, 태그, 스폰된 액터 등)
	AuthoritativeActionPtr->TransferStateFrom(PredictedAction);

	// 예측 인스턴스를 즉시 파괴하지 않고 다음 프레임에 파괴하도록 지연시킵니다.
	if (GetWorld())
	{
		FTimerHandle TempHandle;
		GetWorld()->GetTimerManager().SetTimerForNextTick([PredictedAction]()
			{
				if (PredictedAction && PredictedAction->IsValidLowLevel())
				{
					PredictedAction->MarkAsGarbage();
				}
			});
	}
	else
	{
		PredictedAction->MarkAsGarbage();
	}
}

void UActionSystemComponent::ReHomeActorInstancedAction(FActiveLuxAction& AuthoritativeAction)
{
	// InstancedPerActor의 경우, 예측 인스턴스와 서버 인스턴스는 동일한 객체입니다.
	// 따라서 인스턴스를 찾거나 파괴할 필요 없이, 상태 동기화만 수행하면 됩니다.
	// (상태 동기화는 ReHomePredictedActionTasks 함수의 공통 로직 부분에서 처리됩니다.)
	LogReHomeDetails(AuthoritativeAction, AuthoritativeAction.Action);
}

void UActionSystemComponent::LogReHomeDetails(const FActiveLuxAction& AuthoritativeAction, const ULuxAction* PredictedAction)
{
	if (!AuthoritativeAction.Action || !PredictedAction)
	{
		return;
	}

	FString ClientServerString = GetClientServerContextString(this);
	const ULuxAction* AuthAction = AuthoritativeAction.Action;

	UE_LOG(LogLuxActionSystem, Warning, TEXT("--- %s [ReHome] 태스크 소유권 이전을 시작합니다 (예측 키: %d) ---"), *ClientServerString, AuthoritativeAction.PredictionKey.Key);

	// --- 서버 액션 정보 출력 ---
	UE_LOG(LogLuxActionSystem, Log, TEXT("  %s [서버 액션]: '%s' (클래스: %s)"), *ClientServerString, *GetNameSafe(AuthAction), *AuthAction->GetClass()->GetName());
	UE_LOG(LogLuxActionSystem, Log, TEXT("    - 활성 핸들: %d, 스펙 핸들: %d"), AuthoritativeAction.Handle.Handle, AuthoritativeAction.Spec.Handle.Handle);
	UE_LOG(LogLuxActionSystem, Log, TEXT("    - 소유자: %s, 아바타: %s"), *GetNameSafe(AuthoritativeAction.ActorInfo.OwnerActor.Get()), *GetNameSafe(AuthoritativeAction.ActorInfo.AvatarActor.Get()));
	UE_LOG(LogLuxActionSystem, Log, TEXT("    - 생명 주기: %s, 현재 페이즈: %s"), *UEnum::GetValueAsString(AuthAction->LifecycleState), *AuthAction->CurrentPhaseTag.ToString());

	// --- 예측 액션 정보 출력 ---
	UE_LOG(LogLuxActionSystem, Log, TEXT("  %s [예측 액션]: '%s' (클래스: %s)"), *ClientServerString, *GetNameSafe(PredictedAction), *PredictedAction->GetClass()->GetName());
	UE_LOG(LogLuxActionSystem, Log, TEXT("    - 활성 핸들: %s"), *PredictedAction->GetActiveHandle().ToString());
	UE_LOG(LogLuxActionSystem, Log, TEXT("    - 생명 주기: %s, 현재 페이즈: %s"), *UEnum::GetValueAsString(PredictedAction->LifecycleState), *PredictedAction->CurrentPhaseTag.ToString());

	// 예측 액션의 이전 대상 태스크 목록 출력
	FString PredTasksStr = TEXT("없음");
	if (PredictedAction->ActiveTasks.Num() > 0)
	{
		PredTasksStr.Empty();
		for (const auto& Task : PredictedAction->ActiveTasks)
		{
			PredTasksStr += FString::Printf(TEXT("[%s] "), *GetNameSafe(Task));
		}
	}
	UE_LOG(LogLuxActionSystem, Log, TEXT("    - 소유권 이전 대상 태스크: %s"), *PredTasksStr.TrimEnd());
}

void UActionSystemComponent::EndAction(const FActiveLuxActionHandle& Handle)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FActiveLuxAction* ActiveAction = FindActiveAction(Handle);
	if (ActiveAction && ActiveAction->Action)
	{
		ActiveAction->Action->EndAction();
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("종료 실패: 활성 액션을 찾을 수 없습니다. (핸들: %d)"), Handle.Handle);
	}
}

void UActionSystemComponent::CancelAction(const FActiveLuxActionHandle& Handle)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FActiveLuxAction* ActiveAction = FindActiveAction(Handle);
	if (ActiveAction && ActiveAction->Action)
	{
		ActiveAction->Action->CancelAction();
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("종료 실패: 활성 액션을 찾을 수 없습니다. (핸들: %d)"), Handle.Handle);
	}
}

void UActionSystemComponent::CancelActions(const FGameplayTagContainer* WithTags, const FGameplayTagContainer* WithoutTags)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!WithTags || WithTags->Num() == 0)
	{
		return;
	}

	TArray<FActiveLuxAction> ActiveActionsToTest = ActiveLuxActions.Items;

	for (const FActiveLuxAction& ActionInfo : ActiveActionsToTest)
	{
		const FGameplayTagContainer& ActionTags = ActionInfo.Spec.DynamicTags;

		// WithTags 조건 검사
		if (WithTags && WithTags->Num() > 0 && !ActionTags.HasAny(*WithTags))
		{
			continue;
		}

		// WithoutTags 조건 검사:
		if (WithoutTags && WithoutTags->Num() > 0 && ActionTags.HasAny(*WithoutTags))
		{
			continue;
		}

		CancelAction(ActionInfo.Handle);
	}
}

void UActionSystemComponent::OnActionEnd(const FActiveLuxActionHandle& Handle, bool bWasCancelled)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const int32 FoundIndex = ActiveLuxActions.Items.IndexOfByPredicate([&Handle](const FActiveLuxAction& Action) {
		return Action.Handle == Handle;
		});

	if (FoundIndex == INDEX_NONE)
		return;

	FActiveLuxAction& ActiveLuxAction = ActiveLuxActions.Items[FoundIndex];
	ULuxAction* ActionInstance = ActiveLuxAction.Action.Get();

	// 액션이 이미 소멸 대기 중인지 확인합니다.
	if (!ActionInstance || PendingKillActions.Contains(ActionInstance))
	{
		return;
	}

	UE_LOG(LogLuxActionSystem, Error, TEXT("================>>> [SERVER] Action Ended: %s | Owner: %s | InstancingPolicy: %s | PredictionKey: %d ---"),
		*ActionInstance->GetName(),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(ActionInstance->GetInstancingPolicy()),
		ActiveLuxAction.PredictionKey.Key);

	NotifyActionEnded(ActiveLuxAction.Handle, bWasCancelled);

	// 액션이 'InstancedPerExecution' 정책인 경우 액션 인스턴스를 제거합니다.
	if (ActionInstance && ActionInstance->InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerExecution)
	{
		if (ActionInstance->LifecycleState == ELuxActionLifecycleState::Executing)
		{
			ActionInstance->OnActionEnd(bWasCancelled);
		}

		RemoveReplicatedSubObject(ActionInstance);
		PendingKillActions.Add(ActionInstance);
	}
	else if (ActionInstance && ActionInstance->InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerActor)
	{
		ActionInstance->LifecycleState = ELuxActionLifecycleState::Inactive;
		ActionInstance->ActiveActionHandle = FActiveLuxActionHandle();
	}

	FLuxActionSpec* Spec = FindActionSpecFromHandle(ActiveLuxAction.Spec.Handle);
	if (Spec)
	{
		Spec->ActivationCount = FMath::Max(0, Spec->ActivationCount - 1);
		LuxActionSpecs.MarkItemDirty(*Spec);
	}

	//ActiveActionMap.Remove(Handle);
	ActiveLuxActions.Items.RemoveAt(FoundIndex);
	ActiveLuxActions.MarkArrayDirty();

	// 액션이 'OnGrantAndRemove' 정책으로 부여된 것이라면 종료와 함께 즉시 제거합니다.
	if (ActionInstance && ActionInstance->ActivationPolicy == ELuxActionActivationPolicy::OnGrantAndRemove)
	{
		RemoveAction(Spec->Handle);
	}

	// --- 핵심 로직: 액션 고유 태그 잠금 해제 ---
	if (ActionInstance && ActionInstance->ActionIdentifierTag.IsValid())
	{
		RemoveTag(ActionInstance->ActionIdentifierTag, 1);
	}
}

// ======================================== Action Activation Checks ========================================

bool UActionSystemComponent::CanActivateAction(FLuxActionSpecHandle Handle, FGameplayTagContainer& OutFailureTags) const
{
	const FLuxActionSpec* Spec = FindActionSpecFromHandle(Handle);
	if (!Spec || !Spec->Action)
	{
		return false;
	}

	const ULuxAction* Action = Spec->Action;

	// 1. 차단 태그 체크
	if (HasBlockedTags(*Spec, Action->ActivationBlockedTags, OutFailureTags))
	{
		return false;
	}

	// 2. 필수 태그 체크
	if (!HasRequiredTags(*Spec, Action->ActivationRequiredTags, OutFailureTags))
	{
		return false;
	}

	// 3. 비용 체크
	if (!CheckCost(*Spec, OutFailureTags))
	{
		return false;
	}

	// 4. 쿨다운 체크
	if (!CheckCooldown(*Spec, OutFailureTags))
	{
		return false;
	}

	return true;
}

bool UActionSystemComponent::CheckCooldown(const FLuxActionSpec& Spec, FGameplayTagContainer& OutFailureTags) const
{
	const ULuxAction* Action = Spec.Action;
	if (!Action || !Action->Cooldown)
	{
		return true;
	}

	// 스택이 있는지 확인 - 스택이 있으면 쿨다운을 무시
	const FName RowName = FName(*FString::FromInt(Spec.Level));
	if (const FLuxActionLevelData* LevelDataRow = Action->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("CheckCooldown")))
	{
		const FActionLevelDataBase* LevelData = LevelDataRow->ActionSpecificData.GetPtr<FActionLevelDataBase>();
		if (LevelData && LevelData->MaxChargeStacks > 1)
		{
			FGameplayTag StackTag = Spec.GetStackTag();
			if (GetTagStackCount(StackTag) > 0)
			{
				// 스택이 있으면 쿨다운을 무시하고 사용 가능
				return true;
			}
		}
	}

	// 스택이 없으면 일반적인 쿨다운 체크
	FGameplayTag CooldownTag = Spec.GetCooldownTag();
	if (HasTag(CooldownTag))
	{
		OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_Cooldown);
		return false;
	}

	return true;
}

bool UActionSystemComponent::CheckCost(const FLuxActionSpec& Spec, FGameplayTagContainer& OutFailureTags) const
{
	const ULuxAction* Action = Spec.Action;
	if (!Action) return false;

	// 비용(체력, 마나 등...)을 확인합니다.
	if (Action->Cost)
	{
		if (const ULuxEffect* CostEffectCDO = Action->Cost->GetDefaultObject<ULuxEffect>())
		{
			if (!CanApplyAttributeModifiers(CostEffectCDO->Modifiers, OutFailureTags))
			{
				OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_Cost);
				return false;
			}
		}
	}

	// 추가 비용(아이템 개수, 캐릭터 레벨 등)을 확인합니다.
	for (const TSubclassOf<ULuxActionCost>& CostClass : Action->AdditionalCosts)
	{
		if (const ULuxActionCost* CostCDO = CostClass->GetDefaultObject<ULuxActionCost>())
		{
			if (!CostCDO->CheckCost(this, Spec))
			{
				OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_AdditionalCost);
				return false;
			}
		}
	}

	return true;
}

bool UActionSystemComponent::HasBlockedTags(const FLuxActionSpec& Spec, const FGameplayTagContainer& ActivationBlockedTags, FGameplayTagContainer& OutFailureTags) const
{
	// 액션 고유 태그가 있으면 차단됩니다. 
	// 액션의 중복 실행을 방지하기 위해 사용됩니다.
	if (GrantedTags.ContainsTag(Spec.Action->ActionIdentifierTag))
	{
		OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_AlreadyActive);
		return true;
	}

	// ActivationBlockedTags에 있는 태그 중 하나라도 GrantedTags에 존재하면 차단됩니다.
	for (const FGameplayTag& Tag : ActivationBlockedTags)
	{
		if (GrantedTags.ContainsTag(Tag))
		{
			OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_TagsBlocked);
			return true;
		}
	}

	// 관계 매핑을 통한 차단 태그를 확인합니다.
	if (TagRelationshipMapping)
	{
		FGameplayTagContainer MappedBlockedTags;
		TagRelationshipMapping->GetBlockedActivationTags(Spec.Action->ActionTags, MappedBlockedTags);

		for (const FGameplayTag& Tag : MappedBlockedTags)
		{
			if (GrantedTags.ContainsTag(Tag))
			{
				OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_TagsBlocked);
				return true;
			}
		}
	}

	return false;
}

bool UActionSystemComponent::HasRequiredTags(const FLuxActionSpec& Spec, const FGameplayTagContainer& ActivationRequiredTags, FGameplayTagContainer& OutFailureTags) const
{
	// ActivationRequiredTags에 있는 모든 태그가 GrantedTags에 존재해야 합니다.
	for (const FGameplayTag& Tag : ActivationRequiredTags)
	{
		if (!GrantedTags.ContainsTag(Tag))
		{
			OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_TagsMissing);
			return false;
		}
	}

	// 관계 매핑을 통한 필수 태그를 확인합니다.
	if (TagRelationshipMapping)
	{
		FGameplayTagContainer MappedRequiredTags;
		TagRelationshipMapping->GetRequiredActivationTags(Spec.Action->ActionTags, MappedRequiredTags);

		for (const FGameplayTag& Tag : MappedRequiredTags)
		{
			if (!GrantedTags.ContainsTag(Tag))
			{
				OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_TagsMissing);
				return false;
			}
		}
	}

	return true;
}

// ======================================== Notification Delegates & Events ========================================

void UActionSystemComponent::NotifyActionActivated(const FActiveLuxActionHandle& Handle)
{
	// 핸들을 이용해 실제 액션 정보를 찾아서 델리게이트에 전달합니다.
	FActiveLuxAction* ActiveAction = FindActiveAction(Handle);
	if (ActiveAction && ActiveAction->Action)
	{
		OnActionActivated.Broadcast(ActiveAction->Action);
	}
}

void UActionSystemComponent::NotifyActionFailed(const FLuxActionSpecHandle& Handle, const FGameplayTagContainer& FailureTags)
{
	// Spec 핸들을 이용해 액션 Spec 정보를 찾아서 델리게이트에 전달합니다.
	FLuxActionSpec* Spec = FindActionSpecFromHandle(Handle);
	if (Spec && Spec->Action)
	{
		OnActionFailed.Broadcast(Spec->Action, FailureTags);
		ShowActionFailureMessage(FailureTags);
	}
}

void UActionSystemComponent::NotifyActionEnded(const FActiveLuxActionHandle& Handle, bool bWasCancelled)
{
	FActiveLuxAction* ActiveAction = FindActiveAction(Handle);
	if (ActiveAction && ActiveAction->Action)
	{
		OnActionEnded.Broadcast(ActiveAction->Action, bWasCancelled);
	}

	Client_NotifyActionEnded(Handle, bWasCancelled);
}

void UActionSystemComponent::Client_NotifyActionEnded_Implementation(FActiveLuxActionHandle Handle, bool bWasCancelled)
{
	FActiveLuxAction* ActiveAction = FindActiveAction(Handle);
	if (!ActiveAction)
	{
		return;
	}

	ULuxAction* ActionInstance = ActiveAction->Action;
	if (!ActionInstance)
	{
		return;
	}

	// 액션의 OnActionEnd를 호출하여 클라이언트 측 정리 로직을 실행합니다.
	ActionInstance->OnActionEnd(bWasCancelled);

	// InstancedPerActor 정책의 액션은 재사용되므로 클라이언트에서도 상태를 Inactive로 되돌려야 합니다.
	if (ActionInstance->InstancingPolicy == ELuxActionInstancingPolicy::InstancedPerActor)
	{
		ActionInstance->LifecycleState = ELuxActionLifecycleState::Inactive;
		ActionInstance->ActiveActionHandle = FActiveLuxActionHandle();
	}

	UE_LOG(LogLuxActionSystem, Error, TEXT("================>>> [CLIENT] Action Ended: %s | Owner: %s | InstancingPolicy: %s | PredictionKey: %d ---"),
		*ActionInstance->GetName(),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(ActionInstance->GetInstancingPolicy()),
		ActiveAction->PredictionKey.Key);
}

// ======================================== Event-Driven Activation ========================================

void UActionSystemComponent::TryActivateActionsByEvent(FGameplayTag EventTag, const FContextPayload& Payload)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const TArray<FLuxActionSpecHandle>* TriggeredActionHandles = EventTriggerMap.Find(EventTag);
	if (!TriggeredActionHandles || TriggeredActionHandles->Num() == 0)
	{
		// 해당 이벤트 태그에 연결된 액션이 없다면 아무 작업도 하지 않습니다.
		return;
	}

	for (const FLuxActionSpecHandle& Handle : *TriggeredActionHandles)
	{
		FLuxActionSpec* Spec = FindActionSpecFromHandle(Handle);
		if (!Spec) continue;

		FGameplayTagContainer FailureTags;
		if (CanActivateActionFromEvent(*Spec, Payload, FailureTags))
		{
			Server_TryExecuteAction(Spec->Handle, FLuxPredictionKey());
		}
		else
		{
			NotifyActionFailed(Spec->Handle, FailureTags);
		}
	}
}

bool UActionSystemComponent::CanActivateActionFromEvent(const FLuxActionSpec& Spec, const FContextPayload& Payload, FGameplayTagContainer& OutFailureTags) const
{
	if (!CanActivateAction(Spec.Handle, OutFailureTags))
	{
		return false;
	}

	const ULuxAction* Action = Spec.Action;
	if (const FPayload_GameplayEventData* EventData = Payload.GetData<FPayload_GameplayEventData>(LuxPayloadKeys::GameplayEventData))
	{
		if (Action && EventData->Instigator.IsValid())
		{
			IActionSystemInterface* SourceASCInterface = Cast<IActionSystemInterface>(EventData->Instigator.Get());
			if (!SourceASCInterface) return false;

			const UActionSystemComponent* SourceASC = SourceASCInterface->GetActionSystemComponent();
			if (!SourceASC) return false;

			const FGameplayTagContainer& SourceTags = SourceASC->GetGameplayTags();

			// Source가 Blocked 태그를 가지고 있는지 검사
			if (SourceTags.HasAny(Action->SourceBlockedTags))
			{
				OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_SourceTags);
				return false;
			}

			// Source가 Required 태그를 모두 가지고 있는지 검사
			if (!SourceTags.HasAll(Action->SourceRequiredTags))
			{
				OutFailureTags.AddTag(LuxGameplayTags::Action_Fail_SourceTags);
				return false;
			}
		}
	}

	return true;
}

// ======================================== Helper & Getter Functions ========================================

FLuxActionSpec* UActionSystemComponent::FindActionSpecFromHandle(FLuxActionSpecHandle ActionHandle)
{
	if (!ActionHandle.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("FindActionSpecFromHandle failed: Invalid ActionHandle."));
		return nullptr;
	}


	// 빠른 조회를 위해 Map 에서 검색합니다.
	/*{
		FRWScopeLock ReadLock(ActionSpecsLock, FRWScopeLockType::SLT_ReadOnly);
		FLuxActionSpec** FoundSpecPtr = ActionSpecMap.Find(ActionHandle);
		if (FoundSpecPtr || *FoundSpecPtr)
		{
			return *FoundSpecPtr;
		}
	}*/

	// 맵에 없다면 (리플리케이션 지연 등) 원본 배열에서 검색합니다.
	{
		FRWScopeLock WriteLock(ActionSpecsLock, FRWScopeLockType::SLT_Write);
		for (FLuxActionSpec& Spec : LuxActionSpecs.Items)
		{
			if (Spec.Handle == ActionHandle)
			{
				//ActionSpecMap.Add(ActionHandle, &Spec);
				return &Spec;
			}
		}
	}

	// 맵과 배열 어디에도 없다면 null을 반환합니다.
	return nullptr;
}

const FLuxActionSpec* UActionSystemComponent::FindActionSpecFromHandle(FLuxActionSpecHandle ActionHandle) const
{
	if (!ActionHandle.IsValid())
	{
		return nullptr;
	}

	/*FRWScopeLock ReadLock(ActionSpecsLock, FRWScopeLockType::SLT_ReadOnly);
	FLuxActionSpec* const* FoundSpecPtr = ActionSpecMap.Find(ActionHandle);
	if (FoundSpecPtr || *FoundSpecPtr)
	{
		return *FoundSpecPtr;
	}*/

	for (const FLuxActionSpec& Spec : LuxActionSpecs.Items)
	{
		if (Spec.Handle == ActionHandle)
		{
			return &Spec;
		}
	}

	return nullptr;
}

FActiveLuxAction* UActionSystemComponent::FindActiveAction(const FActiveLuxActionHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}

	//{
	//	// 맵에서 먼저 검색합니다.
	//	FRWScopeLock ReadLock(ActiveActionsLock, FRWScopeLockType::SLT_ReadOnly);
	//	if (FActiveLuxAction** FoundActionPtr = ActiveActionMap.Find(Handle))
	//	{
	//		return *FoundActionPtr;
	//	}
	//}

	{
		// 맵에 없다면 원본 배열을 순회하여 다시 검색합니다.
		FRWScopeLock WriteLock(ActiveActionsLock, FRWScopeLockType::SLT_Write);
		for (FActiveLuxAction& Action : ActiveLuxActions.Items)
		{
			if (Action.Handle == Handle)
			{
				//ActiveActionMap.Add(Handle, &Action);
				UE_LOG(LogLuxActionSystem, Log, TEXT("FindActiveAction: Found action for handle %d via array search."), Handle.Handle);
				return &Action;
			}
		}
	}

	return nullptr;
}

const FActiveLuxAction* UActionSystemComponent::FindActiveAction(const FActiveLuxActionHandle& Handle) const
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}


	/*if (FActiveLuxAction* const* FoundActionPtr = ActiveActionMap.Find(Handle))
	{
		return *FoundActionPtr;
	}*/

	{
		FRWScopeLock ReadLock(ActiveActionsLock, FRWScopeLockType::SLT_ReadOnly);
		for (const FActiveLuxAction& Action : ActiveLuxActions.Items)
		{
			if (Action.Handle == Handle)
			{
				return &Action;
			}
		}
	}

	return nullptr;
}

FActiveLuxAction* UActionSystemComponent::FindActiveActionBySpecHandle(FLuxActionSpecHandle Handle)
{
	if (!Handle.IsValid())
		return nullptr;

	FRWScopeLock ActiveActionsReadLock(ActiveActionsLock, FRWScopeLockType::SLT_ReadOnly);
	return ActiveLuxActions.Items.FindByPredicate([&Handle](const FActiveLuxAction& Action) {
		return Action.Spec.Handle == Handle;
		});
}

FLuxPredictionKey UActionSystemComponent::CreatePredictionKey()
{
	FLuxPredictionKey NewKey;
	NewKey.Key = NextPredictionKeyId;

	NextPredictionKeyId++;

	// int32의 최대값을 넘어 다시 음수로 돌아가는 오버플로우를 방지합니다.
	if (NextPredictionKeyId <= 0)
	{
		NextPredictionKeyId = 1;
	}

	return NewKey;
}

const TArray<FLuxActionSpec>& UActionSystemComponent::GetActionSpecs() const
{
	return LuxActionSpecs.Items;
}

const TArray<FActiveLuxAction>& UActionSystemComponent::GetActiveActions() const
{
	return ActiveLuxActions.Items;
}

FLuxActionSpec* UActionSystemComponent::FindActionSpecByIdentifierTag(const FGameplayTag& IdentifierTag)
{
	if (!IdentifierTag.IsValid())
	{
		return nullptr;
	}

	FRWScopeLock WriteLock(ActionSpecsLock, FRWScopeLockType::SLT_Write);
	for (FLuxActionSpec& Spec : LuxActionSpecs.Items)
	{
		if (Spec.ActionIdentifierTag == IdentifierTag)
		{
			return &Spec;
		}
	}
	return nullptr;
}

void UActionSystemComponent::ShowActionFailureMessage(const FGameplayTagContainer& FailureTags) const
{
	ALuxPlayerState* LuxPS = Cast<ALuxPlayerState>(GetOwnerActor());
	if (LuxPS && LuxPS->IsNetMode(NM_Client) && LuxPS->GetPlayerController())
	{
		// 가장 우선순위가 높은 실패 태그에 따라 메시지를 결정합니다.
		FText FailureMessage;
		if (FailureTags.HasTag(LuxGameplayTags::Action_Fail_Cooldown))
		{
			FailureMessage = FText::FromString(TEXT("아직 사용할 수 없습니다."));
		}
		else if (FailureTags.HasTag(LuxGameplayTags::Action_Fail_Cost))
		{
			FailureMessage = FText::FromString(TEXT("자원이 부족합니다."));
		}
		else if (FailureTags.HasTag(LuxGameplayTags::Action_Fail_TagsBlocked))
		{
			FailureMessage = FText::FromString(TEXT("다른 행동 중에는 사용할 수 없습니다."));
		}
		else if (FailureTags.HasTag(LuxGameplayTags::Action_Fail_TagsMissing))
		{
			FailureMessage = FText::FromString(TEXT("필요한 조건이 충족되지 않았습니다."));
		}
		else
		{
			FailureMessage = FText::FromString(TEXT("지금은 사용할 수 없습니다."));
		}

		// PlayerController를 통해 화면에 알림을 표시합니다.
		if (ALuxPlayerController* PC = LuxPS->GetLuxPlayerController())
		{
			PC->Client_ShowNotification(FailureMessage);
		}
	}
}

/* ======================================== Effect Management ======================================== */

FLuxEffectContextHandle UActionSystemComponent::MakeEffectContext() const
{
	// 핸들 내부에 실제 컨텍스트 데이터 객체를 생성합니다.
	FLuxEffectContextHandle ContextHandle;
	ContextHandle.NewContext();

	if (ContextHandle.IsValid())
	{
		AActor* MyOwner = GetOwnerActor();
		if (MyOwner)
		{
			ContextHandle.SetInstigator(MyOwner->GetInstigator());
		}

		ContextHandle.SetEffectCauser(MyOwner);
		ContextHandle.SetSourceASC(const_cast<UActionSystemComponent*>(this));
	}

	return ContextHandle;
}

FLuxEffectSpecHandle UActionSystemComponent::MakeOutgoingSpec(TSubclassOf<ULuxEffect> EffectClass, float Level, FLuxEffectContextHandle& Context) const
{
	if (Context.IsValid() == false)
	{
		Context = MakeEffectContext();
	}

	if (EffectClass)
	{
		ULuxEffect* GameplayEffect = EffectClass->GetDefaultObject<ULuxEffect>();

		FLuxEffectSpec* NewSpec = new FLuxEffectSpec(GameplayEffect, Level, Context);
		return FLuxEffectSpecHandle(NewSpec);
	}

	return FLuxEffectSpecHandle(nullptr);
}

bool UActionSystemComponent::ApplyEffectSpecToSelf(const FLuxEffectSpecHandle& SpecHandle, FActiveLuxEffectHandle& OutActiveHandle)
{
	return ApplyEffectSpecToTarget(SpecHandle, this, OutActiveHandle);
}

bool UActionSystemComponent::ApplyEffectSpecToTarget(const FLuxEffectSpecHandle& SpecHandle, UActionSystemComponent* TargetASC, FActiveLuxEffectHandle& OutActiveHandle)
{
	if (!SpecHandle.IsValid() || !TargetASC)
	{
		return false; // 실패
	}

	// 서버에서만 실제 적용 로직을 실행합니다.
	if (OwnerActor.IsValid() && OwnerActor->HasAuthority())
	{
		return TargetASC->ApplyEffectSpec_Internal(*SpecHandle.Get(), OutActiveHandle);
	}

	return false;
}

void UActionSystemComponent::RemoveEffect(FActiveLuxEffectHandle Handle)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	OnEffectExpired(Handle);
}

void UActionSystemComponent::RemoveAllActiveEffects()
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	TArray<FActiveLuxEffect> EffectsToRemove = ActiveLuxEffects.Items;
	for (const FActiveLuxEffect& Effect : EffectsToRemove)
	{
		OnEffectExpired(Effect.Handle);
	}

	ActiveLuxEffects.Items.Empty();
}

bool UActionSystemComponent::ApplyEffectSpec_Internal(FLuxEffectSpec& Spec, FActiveLuxEffectHandle& OutActiveHandle)
{
	if (!Spec.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] Spec이 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
	}

	if(OwnerActor.IsValid() == false)
	{
        UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] OwnerActor가 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
	}

	// [1단계] 기본 검증 (태그 기반 검증)
	const ULuxEffect* Template = Spec.EffectTemplate.Get();
	if (!Template) 
	{
        UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] EffectTemplate이 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
	}

	if (HasAny(Template->ApplicationBlockedTags))
	{
        UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][%s] [CheckPrerequisites] 적용 차단 태그(%s)가 있어 이펙트 '%s' 적용에 실패했습니다."), 
            *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *Template->ApplicationBlockedTags.ToString(), *GetNameSafe(Template));
        return false;
	}

	if (HasAll(Template->ApplicationRequiredTags) == false)
	{
        UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][%s] [CheckPrerequisites] 적용 필수 태그(%s)가 없어 이펙트 '%s' 적용에 실패했습니다."), 
            *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *Template->ApplicationRequiredTags.ToString(), *GetNameSafe(Template));
        return false;
	}

	// [2단계] 계산 실행 (기본 검증 통과 후에만 데미지 계산)
    if (!PrepareSpecForApplication(Spec))
    {
        return false; // 계산 실패
    }

	// [3단계] 속성별 상세 검증 (데미지 계산 후, 실제 적용 전)
    if (!CheckApplicationPrerequisites(Spec))
    {
        return false; // AttributeSet에 의해 취소됨
    }

	// [4단계] 이펙트로 제거할 활성화된 LuxEffect 처리
	RemoveActiveEffectsWithTags(Spec.EffectTemplate->RemoveEffectsWithTags);

	// [5단계] 이펙트로 취소할 활성화된 LuxAction 처리
	CancelActions(&Spec.EffectTemplate->CancelActionsWithTags, nullptr);

	// [6단계] 활성 효과 목록 처리 및 핸들 반환
	if (Spec.EffectTemplate->DurationPolicy != ELuxEffectDurationPolicy::Instant)
	{
		OutActiveHandle = AddOrUpdateActiveEffect(Spec);
	}

	// [7단계] 속성 값 실제 변경
	ApplyModifiers(Spec);
    return true;
}

bool UActionSystemComponent::PrepareSpecForApplication(FLuxEffectSpec& Spec)
{
	const ULuxEffect* Template = Spec.EffectTemplate.Get();
	if (!Template) return false;

	UE_LOG(LogLuxActionSystem, Log, TEXT(">> [PrepareSpec] 이펙트 '%s'의 적용 전 계산을 시작합니다."), *GetNameSafe(Template));

	// ExecutionCalculation을 실행하여 OutSpec.CalculatedModifiers를 최종 값으로 채웁니다.
	if (Template->Executions.Num() > 0)
	{
		for (const TSubclassOf<ULuxExecutionCalculation>& ExecClass : Template->Executions)
		{
			if (!ExecClass) 
			{
				UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] ExecutionCalculation이 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
				continue;
			}

			if (ULuxExecutionCalculation* ExecCDO = ExecClass->GetDefaultObject<ULuxExecutionCalculation>())
			{
				UE_LOG(LogLuxActionSystem, Log, TEXT("  -> Execution 로직 '%s'을(를) 실행합니다."), *ExecCDO->GetName());
				ExecCDO->Execute(Spec);
			}
		}
	}

	return true;
}

bool UActionSystemComponent::CheckApplicationPrerequisites(FLuxEffectSpec& Spec)
{
	const ULuxEffect* Template = Spec.EffectTemplate.Get();
	if (!Template) return false;

	// 속성별 상세 검증만 수행 (태그 검증은 이미 ApplyEffectSpec_Internal에서 완료됨)
	for (const FAttributeModifier& Mod : Spec.CalculatedModifiers)
	{
		if(Mod.Attribute.IsValid() == false)
		{
			continue;
		}

		ULuxAttributeSet* AttributeSet = GetAttributeSubobject(Mod.Attribute.GetAttributeSetClass());
		FLuxAttributeData* AttributeData = Mod.Attribute.GetAttributeData(AttributeSet);
		if (!AttributeSet || !AttributeData)
		{
			continue;
		}

		UE_LOG(LogLuxActionSystem, Log, TEXT(">> [CheckPrerequisites] '%s' 속성의 적용 전제 조건을 검사합니다."), *Mod.Attribute.GetName().ToString());

		// 콜백 데이터 생성
		FLuxModCallbackData CallbackData(Spec, Mod.Attribute, *AttributeData, Mod.Magnitude.StaticValue, *this);

		// PreLuxEffectExecute 호출
		if (!AttributeSet->PreLuxEffectExecute(CallbackData))
		{
			UE_LOG(LogLuxActionSystem, Warning, TEXT("<< [CheckPrerequisites] '%s' 속성 검사 실패. 이펙트 적용을 취소합니다."), *Mod.Attribute.GetName().ToString());
			return false;
		}
	}

	return true;
}

void UActionSystemComponent::RemoveActiveEffectsWithTags(const FGameplayTagContainer& TagsToRemove)
{
	if (TagsToRemove.Num() == 0 || ActiveLuxEffects.Items.Num() == 0)
	{
		return;
	}

	TArray<FActiveLuxEffect> EffectsToRemove;
	for (const FActiveLuxEffect& ActiveEffect : ActiveLuxEffects.Items)
	{
		if(ActiveEffect.Spec.EffectTemplate.IsValid() == false)
		{
			UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] EffectTemplate이 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
			continue;
		}

		if (ActiveEffect.Spec.EffectTemplate->EffectTags.HasAny(TagsToRemove))
		{
			EffectsToRemove.Add(ActiveEffect);
		}
	}

	for (const FActiveLuxEffect& EffectToRemove : EffectsToRemove)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[RemoveEffects] 태그(%s) 매칭으로 인해 기존 효과 '%s'를 제거합니다."), *TagsToRemove.ToString(), *GetNameSafe(EffectToRemove.Spec.EffectTemplate.Get()));
		RemoveEffect(EffectToRemove.Handle);
	}
}

void UActionSystemComponent::ApplyModifiers(const FLuxEffectSpec& Spec)
{
	const ULuxEffect* Template = Spec.EffectTemplate.Get();
	if (!Template) return;

	UE_LOG(LogLuxActionSystem, Log, TEXT(">> [ApplyModifiers] 이펙트 '%s'의 Modifier 적용을 시작합니다."), *GetNameSafe(Template));

	// Instant(즉시) 효과는 BaseValue를 직접 변경합니다.
	if (Template->DurationPolicy == ELuxEffectDurationPolicy::Instant)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("  -> 즉시(Instant) 효과로 처리합니다. BaseValue를 직접 변경합니다."));

		for (const FAttributeModifier& Mod : Spec.CalculatedModifiers)
		{
			if (!Mod.Attribute.IsValid())
			{
				continue;
			}

			ULuxAttributeSet* AttributeSet = GetAttributeSubobject(Mod.Attribute.GetAttributeSetClass());
			if (!AttributeSet)
			{
				continue;
			}

			const float CurrentBaseValue = GetNumericAttributeBase(Mod.Attribute);
			float NewBaseValue = CurrentBaseValue;

			switch (Mod.Operation)
			{
			case EModifierOperation::Add:      NewBaseValue += Mod.Magnitude.StaticValue; break;
			case EModifierOperation::Multiply: NewBaseValue *= Mod.Magnitude.StaticValue; break;
			case EModifierOperation::Override: NewBaseValue = Mod.Magnitude.StaticValue;  break;
			}

			UE_LOG(LogLuxActionSystem, Log, TEXT("      -> 적용: '%s', BaseValue: %.2f -> %.2f (연산: %s, 값: %.2f)"),
				*Mod.Attribute.GetName().ToString(), CurrentBaseValue, NewBaseValue,
				*UEnum::GetValueAsString(Mod.Operation), Mod.Magnitude.StaticValue);

			SetNumericAttributeBase(Mod.Attribute, NewBaseValue);

			FLuxAttributeData* AttributeData = Mod.Attribute.GetAttributeData(AttributeSet);
			if (AttributeData)
			{
				FLuxModCallbackData CallbackData(Spec, Mod.Attribute, *AttributeData, Mod.Magnitude.StaticValue, *this);
				AttributeSet->PostLuxEffectExecute(CallbackData);
			}
		}
	}
	// 지속/무한 효과는 모든 활성 효과를 다시 계산하여 CurrentValue를 갱신합니다.
	else
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("  -> 지속(Duration/Infinite) 효과로 처리합니다. CurrentValue를 재계산합니다."));

		TSet<FLuxAttribute> AffectedAttributes;
		for (const FAttributeModifier& Mod : Spec.CalculatedModifiers)
		{
			if (Mod.Attribute.IsValid())
			{
				AffectedAttributes.Add(Mod.Attribute);
			}
		}

		for (const FLuxAttribute& Attr : AffectedAttributes)
		{
			UE_LOG(LogLuxActionSystem, Log, TEXT("    -> 재계산 대상 속성: '%s'"), *Attr.GetName().ToString());
			RecalculateCurrentAttributeValue(Attr);
		}

		for (const FAttributeModifier& Mod : Spec.CalculatedModifiers)
		{
			ULuxAttributeSet* AttributeSet = GetAttributeSubobject(Mod.Attribute.GetAttributeSetClass());
			FLuxAttributeData* AttributeData = Mod.Attribute.GetAttributeData(AttributeSet);
			if (AttributeSet && AttributeData)
			{
				FLuxModCallbackData CallbackData(Spec, Mod.Attribute, *AttributeData, Mod.Magnitude.StaticValue, *this);
				AttributeSet->PostLuxEffectExecute(CallbackData);
			}
		}
	}
	UE_LOG(LogLuxActionSystem, Log, TEXT("<< [ApplyModifiers] Modifier 적용 완료."));
}

FActiveLuxEffectHandle UActionSystemComponent::AddOrUpdateActiveEffect(const FLuxEffectSpec& AppliedSpec)
{
	const ULuxEffect* Template = AppliedSpec.EffectTemplate.Get();
	if (!Template || Template->DurationPolicy == ELuxEffectDurationPolicy::Instant)
	{
		return FActiveLuxEffectHandle();
	}

	FRWScopeLock WriteLock(ActiveLuxEffectsLock, FRWScopeLockType::SLT_Write);

	// 기존 효과 찾기
	FActiveLuxEffectHandle ExistingHandle = FindExistingEffectHandle(AppliedSpec);

	// 기존 효과가 있으면 업데이트, 없으면 새로 추가
	if (ExistingHandle.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][%s] 기존 효과 업데이트: %s"), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *ExistingHandle.ToString());
		return UpdateExistingEffect(ExistingHandle, AppliedSpec);
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][%s] 새로운 효과 추가: %s"), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__), *AppliedSpec.EffectTemplate->GetName());
		return AddNewEffect(AppliedSpec);
	}
}

/* ======================================== Effect Management Helpers ======================================== */

FActiveLuxEffectHandle UActionSystemComponent::FindExistingEffectHandle(const FLuxEffectSpec& AppliedSpec)
{
	const ULuxEffect* Template = AppliedSpec.EffectTemplate.Get();
	if (!Template || Template->StackingType == EEffectStackingType::None)
	{
		return FActiveLuxEffectHandle();
	}

	return FindActiveEffectHandleFromSpec(AppliedSpec);
}

FActiveLuxEffectHandle UActionSystemComponent::UpdateExistingEffect(FActiveLuxEffectHandle ExistingHandle, const FLuxEffectSpec& AppliedSpec)
{
	FActiveLuxEffect* ExistingEffect = FindActiveEffectFromHandle(ExistingHandle);
	if (!ExistingEffect)
	{
		return FActiveLuxEffectHandle();
	}

	const ULuxEffect* Template = AppliedSpec.EffectTemplate.Get();
	if (!Template)
	{
		return FActiveLuxEffectHandle();
	}

	// 스택 수 증가
	const int32 MaxStacks = Template->MaxStacks > 0 ? Template->MaxStacks : 1;
	ExistingEffect->CurrentStacks = FMath::Min(ExistingEffect->CurrentStacks + 1, MaxStacks);
	ExistingEffect->EndTime = GetWorld()->GetTimeSeconds() + AppliedSpec.CalculatedDuration;

	// Replace 정책인 경우 타이머 재설정
	if (Template->StackingType == EEffectStackingType::Replace)
	{
		UpdateEffectTimers(ExistingEffect, AppliedSpec);
	}

	ActiveLuxEffects.MarkItemDirty(*ExistingEffect);
	return ExistingEffect->Handle;
}

FActiveLuxEffectHandle UActionSystemComponent::AddNewEffect(const FLuxEffectSpec& AppliedSpec)
{
	const ULuxEffect* Template = AppliedSpec.EffectTemplate.Get();
	if (!Template)
	{
		return FActiveLuxEffectHandle();
	}

	// 새로운 효과 생성
	FActiveLuxEffect& NewActiveEffect = ActiveLuxEffects.Items.Emplace_GetRef(AppliedSpec);
	NewActiveEffect.StartTime = GetWorld()->GetTimeSeconds();
	NewActiveEffect.EndTime = GetWorld()->GetTimeSeconds() + AppliedSpec.CalculatedDuration;
	NewActiveEffect.CurrentStacks = 1;

	// 동적 태그 부여
	for (const FGameplayTag& Tag : AppliedSpec.DynamicGrantedTags)
	{
		AddTag(Tag, 1);
	}

	// 타이머 설정
	SetupEffectTimers(NewActiveEffect, AppliedSpec);

    // 모든 지속 효과 적용 시 네이티브 델리게이트 브로드캐스트 (서버)
    OnEffectAppliedNative.Broadcast(NewActiveEffect);

	ActiveLuxEffects.MarkItemDirty(NewActiveEffect);
	return NewActiveEffect.Handle;
}

void UActionSystemComponent::UpdateEffectTimers(FActiveLuxEffect* ExistingEffect, const FLuxEffectSpec& AppliedSpec)
{
	if (!ExistingEffect || !GetWorld())
	{
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	// 만료 타이머 재설정
	if (FTimerHandle* TimerHandle = ExpirationTimerMap.Find(ExistingEffect->Handle))
	{
		TimerManager.ClearTimer(*TimerHandle);
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UActionSystemComponent::OnEffectExpired, ExistingEffect->Handle);
		TimerManager.SetTimer(*TimerHandle, Delegate, AppliedSpec.CalculatedDuration, false);
	}

	// 주기 타이머 재설정
	if (FTimerHandle* TimerHandle = PeriodTimerMap.Find(ExistingEffect->Handle))
	{
		TimerManager.ClearTimer(*TimerHandle);
	}

	if (AppliedSpec.CalculatedPeriod > 0.f)
	{
		FTimerHandle NewPeriodTimer;
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UActionSystemComponent::OnPeriodicEffectTick, ExistingEffect->Handle);
		TimerManager.SetTimer(NewPeriodTimer, Delegate, AppliedSpec.CalculatedPeriod, true);
		PeriodTimerMap.Add(ExistingEffect->Handle, NewPeriodTimer);
	}
}

void UActionSystemComponent::SetupEffectTimers(FActiveLuxEffect& NewActiveEffect, const FLuxEffectSpec& AppliedSpec)
{
	if (!GetWorld())
	{
		return;
	}

	const ULuxEffect* Template = AppliedSpec.EffectTemplate.Get();
	if (!Template || Template->DurationPolicy != ELuxEffectDurationPolicy::HasDuration)
	{
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	// 만료 타이머 설정
	if (AppliedSpec.CalculatedDuration > 0.f)
	{
		FTimerHandle NewTimerHandle;
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UActionSystemComponent::OnEffectExpired, NewActiveEffect.Handle);
		TimerManager.SetTimer(NewTimerHandle, Delegate, AppliedSpec.CalculatedDuration, false);
		ExpirationTimerMap.Add(NewActiveEffect.Handle, NewTimerHandle);
	}

	// 주기 타이머 설정
	if (AppliedSpec.CalculatedPeriod > 0.f)
	{
		FTimerHandle NewPeriodTimer;
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UActionSystemComponent::OnPeriodicEffectTick, NewActiveEffect.Handle);
		TimerManager.SetTimer(NewPeriodTimer, Delegate, AppliedSpec.CalculatedPeriod, true);
		PeriodTimerMap.Add(NewActiveEffect.Handle, NewPeriodTimer);
	}
}

FActiveLuxEffectHandle UActionSystemComponent::FindActiveEffectHandleFromSpec(const FLuxEffectSpec& Spec)
{
	for (FActiveLuxEffect& ActiveEffect : ActiveLuxEffects.Items)
	{
		// 동일한 이펙트 템플릿이고 Instigator, EffectCauser, TargetASC, SourceASC 가 동일해야 합니다.
		if (ActiveEffect.Spec.EffectTemplate == Spec.EffectTemplate &&
			ActiveEffect.Spec.ContextHandle == Spec.ContextHandle)
		{
			return ActiveEffect.Handle;
		}
	}

	return FActiveLuxEffectHandle();
}

FActiveLuxEffect* UActionSystemComponent::FindActiveEffectFromHandle(const FActiveLuxEffectHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}

	//{
	//	// 맵에서 먼저 검색합니다.
	//	FRWScopeLock ReadLock(ActiveLuxEffectsLock, FRWScopeLockType::SLT_ReadOnly);
	//	if (FActiveLuxEffect** FoundEffectPtr = ActiveEffectMap.Find(Handle))
	//	{
	//		return *FoundEffectPtr;
	//	}
	//}

	{
		// 맵에 없다면 원본 배열을 순회하여 다시 검색합니다.
		FRWScopeLock ReadLock(ActiveLuxEffectsLock, FRWScopeLockType::SLT_ReadOnly);
		for (FActiveLuxEffect& Effect : ActiveLuxEffects.Items)
		{
			if (Effect.Handle == Handle)
			{
				//ActiveEffectMap.Add(Handle, &Effect);
				UE_LOG(LogLuxActionSystem, Log, TEXT("FindActiveEffectFromHandle: Found effect for handle %s via array search."), *Handle.ToString());
				return &Effect;
			}
		}
	}

	return nullptr;
}

void UActionSystemComponent::OnEffectExpired(FActiveLuxEffectHandle Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	const int32 FoundIndex = ActiveLuxEffects.Items.IndexOfByPredicate([&Handle](const FActiveLuxEffect& Effect)
		{
			return Effect.Handle == Handle;
		});

	if (FoundIndex != INDEX_NONE)
	{
		// 배열에서 제거하기 전에 만료된 이펙트의 데이터를 복사합니다.
		const FActiveLuxEffect ExpiredEffect = ActiveLuxEffects.Items[FoundIndex];
		const TArray<FAttributeModifier> ModsToRecalculate = ExpiredEffect.Spec.CalculatedModifiers;

		ActiveLuxEffects.Items.RemoveAt(FoundIndex);
		ActiveLuxEffects.MarkArrayDirty();

        // 네이티브 이펙트 제거 델리게이트 (서버)
        OnEffectRemovedNative.Broadcast(ExpiredEffect);

		// 이펙트가 부여했던 태그들의 스택 1 감소시킵니다.
		for (const FGameplayTag& Tag : ExpiredEffect.Spec.DynamicGrantedTags)
		{
			RemoveTag(Tag, 1);
		}

		// 이펙트와 관련된 모든 타이머를 정리합니다.
		if (FTimerHandle* Timer = ExpirationTimerMap.Find(Handle))
		{
			GetWorld()->GetTimerManager().ClearTimer(*Timer);
			ExpirationTimerMap.Remove(Handle);
		}
		if (FTimerHandle* Timer = PeriodTimerMap.Find(Handle))
		{
			GetWorld()->GetTimerManager().ClearTimer(*Timer);
			PeriodTimerMap.Remove(Handle);
		}

		// 이펙트 제거의 영향을 받는 모든 속성 값을 재계산합니다.
		TSet<FLuxAttribute> AffectedAttributes;
		for (const FAttributeModifier& Mod : ModsToRecalculate)
		{
			AffectedAttributes.Add(Mod.Attribute);
		}

		for (const FLuxAttribute& Attr : AffectedAttributes)
		{
			RecalculateCurrentAttributeValue(Attr);
		}
	}
}

void UActionSystemComponent::OnPeriodicEffectTick(FActiveLuxEffectHandle Handle)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority() || !Handle.IsValid())
	{
		return;
	}

	FActiveLuxEffect* FoundEffect = ActiveLuxEffects.Items.FindByPredicate([&Handle](const FActiveLuxEffect& Effect) {
		return Effect.Handle == Handle;
		});

	if (FoundEffect)
	{
		for (const FAttributeModifier& Mod : FoundEffect->Spec.CalculatedModifiers)
		{
			const float MagnitudeToApply = Mod.Magnitude.StaticValue * FoundEffect->CurrentStacks;
			ApplyModToAttribute(Mod.Attribute, Mod.Operation, MagnitudeToApply);
		}
	}
}

const TArray<FActiveLuxEffect>& UActionSystemComponent::GetActiveEffects() const
{
	return ActiveLuxEffects.Items;
}

/* ======================================== Cooldown Management ======================================== */

void UActionSystemComponent::CreateCooldownTracker()
{
	FString Caller = GetClientServerContextString(this);
	if (CooldownTracker)
	{
		UE_LOG(LogLuxCooldown, Warning, TEXT("[%s][%s] CooldownTracker 이미 생성되었습니다."), *Caller, ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	CooldownTracker = NewObject<ULuxCooldownTracker>(this);
	if (!CooldownTracker)
	{
		UE_LOG(LogLuxCooldown, Error, TEXT("[%s][%s] CooldownTracker 생성하는 데 실패했습니다."), *Caller, ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	UE_LOG(LogLuxCooldown, Log, TEXT("[%s][%s] CooldownTracker 생성 완료. OwnerASC: %s"), *Caller, ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(this));
	CooldownTracker->Initialize(this);

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		AddReplicatedSubObject(CooldownTracker);
	}
}

void UActionSystemComponent::AdjustCooldownTimerByTag(const FGameplayTag& CooldownTag, float NewRemainingSeconds, float Now)
{
    if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority() || !CooldownTag.IsValid() || NewRemainingSeconds < 0.f)
    {
        return;
    }

    // 활성 쿨다운 이펙트를 찾아 만료 타이머를 재설정
    FActiveLuxEffect* FoundCooldown = nullptr;
    for (FActiveLuxEffect& Effect : ActiveLuxEffects.Items)
    {
        if (Effect.Spec.DynamicEffectTags.HasTag(LuxGameplayTags::Effect_Type_Cooldown))
        {
            for (const FGameplayTag& GrantedTag : Effect.Spec.DynamicGrantedTags)
            {
                if (GrantedTag == CooldownTag)
                {
                    FoundCooldown = &Effect;
                    break;
                }
            }
        }
        if (FoundCooldown) break;
    }

    if (!FoundCooldown)
    {
        return;
    }

    // EndTime 업데이트
    FoundCooldown->EndTime = Now + NewRemainingSeconds;
    ActiveLuxEffects.MarkItemDirty(*FoundCooldown);

    // 타이머 재설정
    if (FTimerHandle* TimerHandle = ExpirationTimerMap.Find(FoundCooldown->Handle))
    {
        GetWorld()->GetTimerManager().ClearTimer(*TimerHandle);

        FTimerDelegate Delegate;
        Delegate.BindUObject(this, &UActionSystemComponent::OnEffectExpired, FoundCooldown->Handle);

        if (NewRemainingSeconds > 0.f)
        {
            GetWorld()->GetTimerManager().SetTimer(*TimerHandle, Delegate, NewRemainingSeconds, false);
        }
        else
        {
            // 즉시 만료 처리
            OnEffectExpired(FoundCooldown->Handle);
        }
    }
}

void UActionSystemComponent::OnRep_EffectAdded(const FActiveLuxEffect& NewEffect)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        return;
    }
	
    // 클라이언트도 복제 시 네이티브 델리게이트 브로드캐스트
    OnEffectAppliedNative.Broadcast(NewEffect);
}

void UActionSystemComponent::OnRep_EffectRemoved(const FActiveLuxEffect& RemovedEffect)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        return;
    }

    // 클라이언트도 복제 시 네이티브 델리게이트 브로드캐스트
    OnEffectRemovedNative.Broadcast(RemovedEffect);
}

/* ======================================== Cue Management ======================================== */

void UActionSystemComponent::ExecuteGameplayCue(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context)
{
	if (OwnerActor.IsValid() && OwnerActor->HasAuthority())
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[ASC::ExecuteGameplayCue] Called on SERVER. Executing as multicast. CueTag: %s"), *CueTag.ToString());
		NetMulticast_ExecuteCue(Target, CueTag, Context, FLuxPredictionKey());
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[ASC::ExecuteGameplayCue] Called on CLIENT. Executing locally and sending RPC. CueTag: %s"), *CueTag.ToString());
		FLuxPredictionKey PredictionKey = CreatePredictionKey();
		ExecuteGameplayCue_Local(Target, CueTag, Context, PredictionKey);
		Server_ExecuteGameplayCue(Target, CueTag, Context, PredictionKey);
	}
}

void UActionSystemComponent::ExecuteGameplayCue_Local(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context, FLuxPredictionKey& PredictionKey)
{
	if (CanExecuteCue(CueTag) == false)
	{
		return;
	}

	PendingCues.Add(PredictionKey, CueTag);

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (ULuxCueManager* CueManager = GameInstance->GetSubsystem<ULuxCueManager>())
		{
			CueManager->HandleCue(Target, CueTag, Context);
		}
	}
}

void UActionSystemComponent::Server_ExecuteGameplayCue_Implementation(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context, FLuxPredictionKey PredictionKey)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (CanExecuteCue(CueTag) == false)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("Server_ExecuteGameplayCue failed: Cannot execute cue %s."), *CueTag.ToString());
		Client_RejectCuePrediction(PredictionKey);
		return;
	}

	NetMulticast_ExecuteCue(Target, CueTag, Context, PredictionKey);
}

void UActionSystemComponent::NetMulticast_ExecuteCue_Implementation(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context, FLuxPredictionKey PredictionKey)
{
	// 내가 예측했던 Cue가 맞다면 대기 목록에서 제거합니다.
	if (PendingCues.Contains(PredictionKey))
	{
		PendingCues.Remove(PredictionKey);
		return;

	}

	// 예측과 관련 없는 Cue (다른 플레이어의 Cue 등)라면 효과를 재생합니다.
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (ULuxCueManager* CueManager = GameInstance->GetSubsystem<ULuxCueManager>())
		{
			CueManager->HandleCue(Target, CueTag, Context);
		}
	}
}

void UActionSystemComponent::StopGameplayCue(AActor* Target, FGameplayTag CueTag)
{
	if (OwnerActor.IsValid() && OwnerActor->HasAuthority())
	{
		NetMulticast_StopGameplayCue(Target, CueTag);
	}
	else
	{
		// 클라이언트는 서버에 RPC를 보냅니다.
		Server_StopGameplayCue(Target, CueTag);
	}
}

void UActionSystemComponent::Server_StopGameplayCue_Implementation(AActor* Target, FGameplayTag CueTag)
{
	NetMulticast_StopGameplayCue(Target, CueTag);
}

void UActionSystemComponent::NetMulticast_StopGameplayCue_Implementation(AActor* Target, FGameplayTag CueTag)
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (ULuxCueManager* CueManager = GameInstance->GetSubsystem<ULuxCueManager>())
		{
			// CueManager를 통해 해당 태그의 모든 큐를 중지시킵니다.
			CueManager->StopAllCuesByTag(Target, CueTag);
		}
	}
}

void UActionSystemComponent::Client_RejectCuePrediction_Implementation(const FLuxPredictionKey& PredictionKey)
{
	UE_LOG(LogLuxActionSystem, Warning, TEXT("Client_RejectCuePrediction: PredictionKey=%d"), PredictionKey.Key);

	FGameplayTag* FoundCueTag = PendingCues.Find(PredictionKey);
	if (!FoundCueTag)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("Client_RejectCuePrediction failed: PredictionKey %d not found in PendingCues."), PredictionKey.Key);
		return;
	}

	FGameplayTag CueTagToRemove = *FoundCueTag;
	PendingCues.Remove(PredictionKey);

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (ULuxCueManager* CueManager = GameInstance->GetSubsystem<ULuxCueManager>())
		{
			CueManager->StopCue(GetAvatarActor(), CueTagToRemove);
		}
	}
}

bool UActionSystemComponent::CanExecuteCue(FGameplayTag CueTag) const
{
	if (HasTag(CueTag))
	{
		return false;
	}

	return true;
}

/* ======================================== Attribute Management ======================================== */

ULuxAttributeSet* UActionSystemComponent::GetAttributeSet(TSubclassOf<ULuxAttributeSet> AttributeSetClass) const
{
	return GetAttributeSubobject(AttributeSetClass);
}

void UActionSystemComponent::AddAttributeSet(ULuxAttributeSet* Attribute)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!::IsValid(Attribute))
	{
		return;
	}

	if (GreantedAttributes.Find(Attribute) == INDEX_NONE)
	{
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
		{
			AddReplicatedSubObject(Attribute);
		}

		GreantedAttributes.Add(Attribute);
		SetSpawnedAttributesDirty();
	}
}

void UActionSystemComponent::RemoveAttribute(ULuxAttributeSet* Attribute)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!::IsValid(Attribute))
	{
		return;
	}

	if (GreantedAttributes.Contains(Attribute))
	{
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
		{
			RemoveReplicatedSubObject(Attribute);
		}

		Attribute->MarkAsGarbage();
		GreantedAttributes.Remove(Attribute);
		SetSpawnedAttributesDirty();
	}
}

void UActionSystemComponent::RemoveAllAttributes()
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	TArray<ULuxAttributeSet*> AttributesToRemove = GreantedAttributes;
	for (ULuxAttributeSet* Set : AttributesToRemove)
	{
		RemoveAttribute(Set);
	}

	GreantedAttributes.Empty();
}

ULuxAttributeSet* UActionSystemComponent::GetAttributeSubobject(const TSubclassOf<ULuxAttributeSet> AttributeClass) const
{
	for (ULuxAttributeSet* Set : GetSpawnedAttributes())
	{
		if (Set && Set->IsA(AttributeClass))
		{
			return Set;
		}
	}
	return nullptr;
}

ULuxAttributeSet* UActionSystemComponent::GetAttributeSubobjectChecked(const TSubclassOf<ULuxAttributeSet> AttributeClass) const
{
	ULuxAttributeSet* Set = GetAttributeSubobject(AttributeClass);
	check(Set);
	return Set;
}

ULuxAttributeSet* UActionSystemComponent::GetOrCreateAttributeSet(TSubclassOf<ULuxAttributeSet> AttributeClass)
{
	AActor* OwningActor = GetOwner();
	ULuxAttributeSet* MyAttributes = nullptr;
	if (OwningActor && AttributeClass)
	{
		MyAttributes = GetAttributeSubobject(AttributeClass);
		if (!MyAttributes)
		{
			ULuxAttributeSet* Attributes = NewObject<ULuxAttributeSet>(OwningActor, AttributeClass);
			AddAttributeSet(Attributes);
			MyAttributes = Attributes;
		}
	}

	return MyAttributes;
}

const TArray<ULuxAttributeSet*>& UActionSystemComponent::GetSpawnedAttributes() const
{
	return GreantedAttributes;
}

bool UActionSystemComponent::CanApplyAttributeModifiers(const TArray<FAttributeModifier>& Modifiers, FGameplayTagContainer& OutFailureTags) const
{
	if (Modifiers.Num() == 0)
	{
		return true;
	}

	for (const FAttributeModifier& Mod : Modifiers)
	{
		const FLuxAttribute& AttributeToCheck = Mod.Attribute;
		if (!AttributeToCheck.IsValid()) continue;

		const ULuxAttributeSet* Set = GetAttributeSet(AttributeToCheck.GetAttributeSetClass());
		if (!Set) return false;

		const FLuxAttributeData* Data = AttributeToCheck.GetAttributeData(Set);
		if (!Data) return false;

		const float CurrentValue = Data->GetCurrentValue();
		float CostMagnitude = 0.f;

		switch (Mod.Operation)
		{
		case EModifierOperation::Add:
			if (CurrentValue + Mod.Magnitude.StaticValue < 0.f)
			{
				FString FailTag = "Action.Fail.Cost." + AttributeToCheck.GetName().ToString();
				OutFailureTags.AddTag(FGameplayTag::RequestGameplayTag(FName(FailTag)));
				return false; // 자원 부족
			}
			break;

		case EModifierOperation::Multiply:
		case EModifierOperation::Override:
			// 지금은 이 연산들을 비용으로 간주하지 않고 항상 통과시킵니다.
			break;
		}
	}

	return true;
}

void UActionSystemComponent::ApplyModToAttribute(const FLuxAttribute& Attribute, EModifierOperation Operation, float Magnitude)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	ULuxAttributeSet* Set = GetAttributeSubobject(Attribute.GetAttributeSetClass());
	FLuxAttributeData* Data = Set ? Attribute.GetAttributeData(Set) : nullptr;

	if (!Data) return;

	const float OldValue = Data->GetCurrentValue();
	float NewValue = OldValue;

	switch (Operation)
	{
	case EModifierOperation::Add:
		NewValue += Magnitude;
		break;
	case EModifierOperation::Multiply:
		NewValue *= Magnitude;
		break;
	case EModifierOperation::Override:
		NewValue = Magnitude;
		break;
	}

	// 최종적으로 적용될 값을 PreAttributeChange를 통해 보정합니다.
	Set->PreAttributeChange(Attribute, NewValue);

	// 값이 실제로 변경되었다면, CurrentValue를 설정하고 PostAttributeChange 이벤트를 호출합니다.
	if (!FMath::IsNearlyEqual(OldValue, NewValue))
	{
		Data->SetCurrentValue(NewValue);
		Set->PostAttributeChange(Attribute, OldValue, NewValue);
	}
}

void UActionSystemComponent::RecalculateCurrentAttributeValue(const FLuxAttribute& Attribute)
{
	ULuxAttributeSet* Set = GetAttributeSubobject(Attribute.GetAttributeSetClass());
	FLuxAttributeData* Data = Set ? Attribute.GetAttributeData(Set) : nullptr;

	if (!Data)
	{
		return;
	}

	const float OldCurrentValue = Data->GetCurrentValue();
	float NewCurrentValue = Data->GetBaseValue();

	float OverrideValue = -1.f;
	bool bHasOverride = false;

	for (const FActiveLuxEffect& ActiveEffect : ActiveLuxEffects.Items)
	{
		if (ActiveEffect.Spec.EffectTemplate->DurationPolicy == ELuxEffectDurationPolicy::Instant)
			continue;

		for (const FAttributeModifier& Mod : ActiveEffect.Spec.CalculatedModifiers)
		{
			if (Mod.Attribute == Attribute && Mod.Operation == EModifierOperation::Override)
			{
				OverrideValue = Mod.Magnitude.StaticValue; // 스택은 반영 X
				bHasOverride = true;
			}
		}
	}

	if (bHasOverride)
	{
		NewCurrentValue = OverrideValue;
	}
	else
	{
		// 곱셈 및 나눗셈 연산 처리 
		float TotalMultiplier = 1.0f;
		for (const FActiveLuxEffect& ActiveEffect : ActiveLuxEffects.Items)
		{
			if (ActiveEffect.Spec.EffectTemplate->DurationPolicy == ELuxEffectDurationPolicy::Instant) continue;
			for (const FAttributeModifier& Mod : ActiveEffect.Spec.CalculatedModifiers)
			{
				if (Mod.Attribute == Attribute && Mod.Operation == EModifierOperation::Multiply)
				{
					// 스택이 적용된 배율을 곱합니다.
					// 예: 10% 증가는 1.1, 2스택이면 (1.0 + (0.1 * 2)) = 1.2배. 또는 (1.1^2) 등 정책에 따라 결정.
					// 여기서는 간단한 덧셈 기반 배율로 가정합니다. 1.0f + ( (Mod.Magnitude - 1.0f) * 스택 )
					TotalMultiplier += (Mod.Magnitude.StaticValue - 1.0f) * ActiveEffect.CurrentStacks;
				}
			}
		}
		NewCurrentValue *= TotalMultiplier;

		// 덧셈 및 뺄셈 연산 처리 ---
		float TotalAddition = 0.0f;
		for (const FActiveLuxEffect& ActiveEffect : ActiveLuxEffects.Items)
		{
			if (ActiveEffect.Spec.EffectTemplate->DurationPolicy == ELuxEffectDurationPolicy::Instant) continue;
			for (const FAttributeModifier& Mod : ActiveEffect.Spec.CalculatedModifiers)
			{
				if (Mod.Attribute == Attribute && Mod.Operation == EModifierOperation::Add)
				{
					TotalAddition += Mod.Magnitude.StaticValue * ActiveEffect.CurrentStacks;
				}
			}
		}
		NewCurrentValue += TotalAddition;
	}

	// 최종 계산된 값이 변경되었을 때만 이벤트를 호출하고 값을 설정합니다.
	if (!FMath::IsNearlyEqual(OldCurrentValue, NewCurrentValue))
	{
		Set->PreAttributeChange(Attribute, NewCurrentValue);
		Data->SetCurrentValue(NewCurrentValue);
		Set->PostAttributeChange(Attribute, OldCurrentValue, NewCurrentValue);
	}
}

void UActionSystemComponent::RecalculateAllCurrentAttributeValues()
{
	for (ULuxAttributeSet* Set : GreantedAttributes)
	{
		if (!Set) continue;

		for (TFieldIterator<FProperty> PropIt(Set->GetClass()); PropIt; ++PropIt)
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(*PropIt))
			{
				if (StructProp->Struct == FLuxAttributeData::StaticStruct())
				{
					FLuxAttribute Attr(StructProp);
					RecalculateCurrentAttributeValue(Attr);
				}
			}
		}
	}
}

void UActionSystemComponent::SetNumericAttributeBase(const FLuxAttribute& Attribute, float NewBaseValue)
{
	if (!Attribute.IsValid())
	{
		return;
	}

	ULuxAttributeSet* Set = GetAttributeSubobject(Attribute.GetAttributeSetClass());
	if (!Set)
	{
		return;
	}

	FLuxAttributeData* Data = Attribute.GetAttributeData(Set);
	if (!Data)
	{
		return;
	}

	const float OldValue = Data->GetBaseValue();

	float TempNewValue = NewBaseValue;
	Set->PreAttributeBaseChange(Attribute, TempNewValue);
	Data->SetBaseValue(TempNewValue);
	Set->PostAttributeBaseChange(Attribute, OldValue, Data->GetBaseValue());

	RecalculateCurrentAttributeValue(Attribute);
}

float UActionSystemComponent::GetNumericAttributeBase(const FLuxAttribute& Attribute) const
{
	const ULuxAttributeSet* Set = GetAttributeSet(Attribute.GetAttributeSetClass());
	if (Set)
	{
		const FLuxAttributeData* Data = Attribute.GetAttributeData(Set);
		if (Data)
		{
			return Data->GetBaseValue();
		}
	}

	return 0.0f;
}

void UActionSystemComponent::SetSpawnedAttributesDirty()
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, GreantedAttributes, this);
}

/* ======================================== Input Management ======================================== */

void UActionSystemComponent::ClearActionInput()
{


}

void UActionSystemComponent::ActionInputTagPressed(FLuxActionSpec& Spec)
{
	// 리플리케이트된 이벤트를 호출하여 'InputPressed' 상태를 알립니다.
	InvokeReplicatedEvent(EActionReplicatedEvent::InputPressed, Spec.Handle);
}

void UActionSystemComponent::ActionInputTagReleased(FLuxActionSpec& Spec)
{
	// 리플리케이트된 이벤트를 호출하여 'InputReleased' 상태를 알립니다.
	InvokeReplicatedEvent(EActionReplicatedEvent::InputReleased, Spec.Handle);
}

void UActionSystemComponent::InvokeReplicatedEvent(EActionReplicatedEvent EventType, FLuxActionSpecHandle SpecHandle)
{
	// 서버에서 직접 이벤트를 처리
	if (GetOwner()->HasAuthority())
	{
		ServerInvokeReplicatedEvent(EventType, SpecHandle);
	}
	// 클라이언트에서 서버로 RPC 전송
	else
	{
		ServerInvokeReplicatedEvent(EventType, SpecHandle);
	}
}

bool UActionSystemComponent::ServerInvokeReplicatedEvent_Validate(EActionReplicatedEvent EventType, FLuxActionSpecHandle SpecHandle)
{
	return true;
}

void UActionSystemComponent::ServerInvokeReplicatedEvent_Implementation(EActionReplicatedEvent EventType, FLuxActionSpecHandle SpecHandle)
{
	if (FLuxActionSpec* ActionSpec = FindActionSpecFromHandle(SpecHandle))
	{
		if (ActionSpec->Action)
		{
			// 실제 액션 인스턴스에 이벤트가 도착했음을 알립니다.
			ActionSpec->Action->OnReplicatedEvent(EventType);
		}
	}
}

void UActionSystemComponent::ActionInputTagPressed(FGameplayTag InputTag)
{
	FRWScopeLock WriteLock(InputHandlesLock, FRWScopeLockType::SLT_Write);

	for (FLuxActionSpec& Spec : LuxActionSpecs.Items)
	{
		if (Spec.Action && Spec.DynamicTags.HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.AddUnique(Spec.Handle);
		}
	}

	OnLocalInputTagPressed.Broadcast(InputTag);
}

void UActionSystemComponent::ActionInputTagReleased(FGameplayTag InputTag)
{
	FRWScopeLock WriteLock(InputHandlesLock, FRWScopeLockType::SLT_Write);

	for (FLuxActionSpec& Spec : LuxActionSpecs.Items)
	{
		if (Spec.Action && Spec.DynamicTags.HasTagExact(InputTag))
		{
			InputHeldSpecHandles.Remove(Spec.Handle);
			InputReleasedSpecHandles.AddUnique(Spec.Handle);
		}
	}

	OnLocalInputTagReleased.Broadcast(InputTag);
}

void UActionSystemComponent::ProcessActionInput(float DeltaTime, bool bGamePaused)
{
	FRWScopeLock ReadLock(InputHandlesLock, FRWScopeLockType::SLT_ReadOnly);

	TArray<FLuxActionSpecHandle> ActionToActivate;
	ActionToActivate.Reset();

	// 입력이 계속 눌리는(hold) 동안 활성화되는 어빌리티를 처리합니다.
	for (const FLuxActionSpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FLuxActionSpec* ActionSpec = FindActionSpecFromHandle(SpecHandle))
		{
			if (ActionSpec->Action && !ActionSpec->IsActive())
			{
				const ULuxAction* LuxActionCDO = Cast<ULuxAction>(ActionSpec->Action);
				if (LuxActionCDO && LuxActionCDO->GetActivationPolicy() == ELuxActionActivationPolicy::WhileInputActive)
				{
					ActionToActivate.AddUnique(ActionSpec->Handle);
				}
			}
		}
	}

	// 이 프레임에 입력이 눌린(pressed) 어빌리티를 처리합니다.
	for (const FLuxActionSpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FLuxActionSpec* ActionSpec = FindActionSpecFromHandle(SpecHandle))
		{
			if (ActionSpec->Action)
			{
				ActionSpec->InputPressed = true;

				if (ActionSpec->IsActive())
				{
					// 이미 활성화된 어빌리티에 입력 이벤트를 보냅니다.
					ActionInputTagPressed(*ActionSpec);
				}
				else
				{
					const ULuxAction* LuxActionCDO = Cast<ULuxAction>(ActionSpec->Action);
					if (LuxActionCDO && LuxActionCDO->GetActivationPolicy() == ELuxActionActivationPolicy::OnInputTriggered)
					{
						ActionToActivate.AddUnique(ActionSpec->Handle);
					}
				}
			}
		}
	}

	// 수집된 모든 입력에 따라 활성화될 어빌리티들을 일괄적으로 시도합니다.
	// 다른 입력으로 어빌리티가 활성화될 때, 해당 입력 이벤트가 중복으로 전송되는 것을 방지하기 위함입니다.
	for (const FLuxActionSpecHandle& ActionSpecHandle : ActionToActivate)
	{
		TryExecuteAction(ActionSpecHandle);
	}

	// 이 프레임에 입력이 떼어진(released) 어빌리티를 처리합니다.
	for (const FLuxActionSpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FLuxActionSpec* ActionSpec = FindActionSpecFromHandle(SpecHandle))
		{
			if (ActionSpec->Action)
			{
				ActionSpec->InputPressed = false;

				if (ActionSpec->IsActive())
				{
					// 활성화된 어빌리티에 입력 해제 이벤트를 보냅니다.
					ActionInputTagReleased(*ActionSpec);
				}
			}
		}
	}

	// 캐시된 입력 핸들을 초기화합니다.
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

/*======================================== Tag Relationship ======================================== */

void UActionSystemComponent::SetTagRelationshipMapping(ULuxActionTagRelationshipMapping* NewMapping)
{
	TagRelationshipMapping = NewMapping;
}

void UActionSystemComponent::Server_AddTag_Implementation(const FGameplayTag& Tag, int32 StackCount)
{
	AddTag(Tag, StackCount);
}

void UActionSystemComponent::AddTag(const FGameplayTag& Tag, int32 StackCount)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority() || StackCount <= 0) return;

	const int32 OldCount = GrantedTags.GetStackCount(Tag);
	GrantedTags.AddStack(Tag, StackCount);
	GrantedTags.MarkArrayDirty();
	const int32 NewCount = GrantedTags.GetStackCount(Tag);

	// 변경이 있었을 경우에만 델리게이트를 호출합니다.
	if (OldCount != NewCount)
	{
		OnGameplayTagStackChanged.Broadcast(Tag, OldCount, NewCount);
	}
}

void UActionSystemComponent::AddTags(const FGameplayTagContainer& TagContainer, int32 StackCount)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority() || StackCount <= 0) return;

	for (const FGameplayTag& Tag : TagContainer)
	{
		const int32 OldCount = GrantedTags.GetStackCount(Tag);
		GrantedTags.AddStack(Tag, StackCount);
		GrantedTags.MarkArrayDirty();
		const int32 NewCount = GrantedTags.GetStackCount(Tag);

		if (OldCount != NewCount)
		{
			OnGameplayTagStackChanged.Broadcast(Tag, OldCount, NewCount);
		}
	}
}

void UActionSystemComponent::Server_RemoveTag_Implementation(const FGameplayTag& Tag, int32 StackCount)
{
	RemoveTag(Tag, StackCount);
}

void UActionSystemComponent::RemoveTag(const FGameplayTag& Tag, int32 StackCount)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority() || StackCount <= 0) return;

	const int32 OldCount = GrantedTags.GetStackCount(Tag);
	GrantedTags.RemoveStack(Tag, StackCount);
	GrantedTags.MarkArrayDirty();
	const int32 NewCount = GrantedTags.GetStackCount(Tag);

	if (OldCount != NewCount)
	{
		OnGameplayTagStackChanged.Broadcast(Tag, OldCount, NewCount);
	}
}

void UActionSystemComponent::RemoveTags(const FGameplayTagContainer& TagContainer, int32 StackCount)
{
	if (!OwnerActor.IsValid() || !GetOwner()->HasAuthority() || StackCount <= 0) return;

	for (const FGameplayTag& Tag : TagContainer)
	{
		const int32 OldCount = GrantedTags.GetStackCount(Tag);
		GrantedTags.RemoveStack(Tag, StackCount);
		GrantedTags.MarkArrayDirty();
		const int32 NewCount = GrantedTags.GetStackCount(Tag);

		if (OldCount != NewCount)
		{
			OnGameplayTagStackChanged.Broadcast(Tag, OldCount, NewCount);
		}
	}
}

bool UActionSystemComponent::HasTag(const FGameplayTag& Tag) const
{
	return GrantedTags.ContainsTag(Tag);
}

bool UActionSystemComponent::HasAny(const FGameplayTagContainer& TagContainer) const
{
	for (const FGameplayTag& Tag : TagContainer)
	{
		if (GrantedTags.ContainsTag(Tag))
		{
			return true;
		}
	}

	return false;
}

bool UActionSystemComponent::HasAll(const FGameplayTagContainer& TagContainer) const
{
	for (const FGameplayTag& Tag : TagContainer)
	{
		if (!GrantedTags.ContainsTag(Tag))
		{
			return false;
		}
	}

	return true;
}

int32 UActionSystemComponent::GetTagStackCount(const FGameplayTag& Tag) const
{
	return GrantedTags.GetStackCount(Tag);
}

FGameplayTagContainer UActionSystemComponent::GetGameplayTags() const
{
	return GrantedTags.GetExplicitGameplayTags();
}

/* ======================================== Gameplay Event System ======================================== */

void UActionSystemComponent::HandleGameplayEvent(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	if (EventTag.IsValid())
	{
		// 이벤트에 반응하는 액션을 찾아 활성화를 시도합니다.
		TryActivateActionsByEvent(EventTag, Payload);
		BroadcastGameplayEventToSubscribers(EventTag, Payload);
	}
}

void UActionSystemComponent::SubscribeToGameplayEvent(const FGameplayTag& EventTag, const FScriptDelegate& Delegate)
{
	if (EventTag.IsValid() == false)
	{
		return;
	}

	FOnGameplayEvent& EventDelegate = EventSubscriptions.FindOrAdd(EventTag);
	if (!EventDelegate.Contains(Delegate))
	{
		EventDelegate.Add(Delegate);
	}
}

void UActionSystemComponent::UnsubscribeFromGameplayEvent(const FGameplayTag& EventTag, const FScriptDelegate& Delegate)
{
	if (EventTag.IsValid() == false)
	{
		return;
	}

	FOnGameplayEvent* EventDelegate = EventSubscriptions.Find(EventTag);
	if (!EventDelegate)
	{
		return;
	}

	EventDelegate->Remove(Delegate);
}

void UActionSystemComponent::BroadcastGameplayEventToSubscribers(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
    if (FOnGameplayEvent* EventDelegate = EventSubscriptions.Find(EventTag))
    {
		EventDelegate->Broadcast(EventTag, Payload);
    }
}

// ======================================== Task Event Handling ========================================

void UActionSystemComponent::Server_ReceiveTaskEvent_Implementation(FActiveLuxActionHandle ActionHandle, const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	if (!OwnerActor.IsValid() || !OwnerActor->HasAuthority())
	{
		return;
	}

	FActiveLuxAction* FoundAction = FindActiveAction(ActionHandle);
	if (FoundAction && FoundAction->Action)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[Server] Received Task Event '%s' for Action '%s'."), *EventTag.ToString(), *FoundAction->Action->GetName());
		FoundAction->Action->PostTaskEvent(EventTag, Payload);
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("[Server] Action not found for handle '%s'. Task event '%s' will be processed when server action is ready."), 
			*ActionHandle.ToString(), *EventTag.ToString());
	}
}

