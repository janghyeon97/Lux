// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask_WaitGameplayEvent.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

ULuxActionTask_WaitGameplayEvent* ULuxActionTask_WaitGameplayEvent::WaitGameplayEvent(ULuxAction* InOwningAction, FGameplayTag EventTag)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[WaitGameplayEvent] WaitGameplayEventTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"WaitGameplayEventTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_WaitGameplayEvent* NewTask = NewObject<ULuxActionTask_WaitGameplayEvent>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("WaitGameplayEventTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("WaitGameplayEvent");
	NewTask->OwningAction = InOwningAction;
	NewTask->TagToWaitFor = EventTag;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_WaitGameplayEvent::InitializeFromStruct(const FInstancedStruct& Struct)
{
	if (const FWaitGameplayEventParams* Params = Struct.GetPtr<FWaitGameplayEventParams>())
	{
		TagToWaitFor = Params->EventTagToWait;
	}
}

void ULuxActionTask_WaitGameplayEvent::OnActivated()
{
	Super::OnActivated();

	UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
	if (ASC)
	{
		// 게임플레이 이벤트 구독
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxActionTask_WaitGameplayEvent, OnGameplayEventCallback));
		ASC->SubscribeToGameplayEvent(TagToWaitFor, Delegate);
	}
}

void ULuxActionTask_WaitGameplayEvent::OnEnded(bool bSuccess)
{
	UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
	if (ASC)
	{
		// 게임플레이 이벤트 구독 해제
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxActionTask_WaitGameplayEvent, OnGameplayEventCallback));
		ASC->UnsubscribeFromGameplayEvent(TagToWaitFor, Delegate);
	}

	Super::OnEnded(bSuccess);
}

void ULuxActionTask_WaitGameplayEvent::OnBeforeReHome()
{
	UActionSystemComponent* OldASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
	if (OldASC)
	{
		// 게임플레이 이벤트 구독 해제
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxActionTask_WaitGameplayEvent, OnGameplayEventCallback));
		OldASC->UnsubscribeFromGameplayEvent(TagToWaitFor, Delegate);
	}
}

void ULuxActionTask_WaitGameplayEvent::OnAfterReHome()
{
	UActionSystemComponent* NewASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
	if (NewASC)
	{
		// 게임플레이 이벤트 구독
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULuxActionTask_WaitGameplayEvent, OnGameplayEventCallback));
		NewASC->SubscribeToGameplayEvent(TagToWaitFor, Delegate);
	}
}

void ULuxActionTask_WaitGameplayEvent::OnGameplayEventCallback(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	// 이미 다른 이유로 태스크가 종료되었다면 아무것도 하지 않습니다.
	if (bIsEnded) return;

	if (EventTag.MatchesTag(TagToWaitFor))
	{
		FContextPayload TaskPayload;
		OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_GameplayEvent_Received, TaskPayload);
		EndTask(true);
	}
}