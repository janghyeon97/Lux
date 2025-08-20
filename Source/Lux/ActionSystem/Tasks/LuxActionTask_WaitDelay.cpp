// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask_WaitDelay.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

ULuxActionTask_WaitDelay* ULuxActionTask_WaitDelay::WaitDelay(ULuxAction* InOwningAction, float WaitDuration)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[WaitDelay] WaitDelayTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"WaitDelayTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_WaitDelay* NewTask = NewObject<ULuxActionTask_WaitDelay>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("WaitDelayTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("WaitDelay");
	NewTask->OwningAction = InOwningAction;
	NewTask->WaitDuration = WaitDuration;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_WaitDelay::InitializeFromStruct(const FInstancedStruct& Struct)
{
	if (const FWaitDelayParams* Params = Struct.GetPtr<FWaitDelayParams>())
	{
		WaitDuration = Params->Duration.GetValue(OwningAction.Get());
	}
}

void ULuxActionTask_WaitDelay::OnActivated()
{
	Super::OnActivated();

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &ULuxActionTask_WaitDelay::OnTimeExpired, WaitDuration, false);
	}
}

void ULuxActionTask_WaitDelay::OnEnded(bool bSuccess)
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}

	Super::OnEnded(bSuccess);
}

void ULuxActionTask_WaitDelay::OnTimeExpired()
{
	// 이미 다른 이유로 태스크가 종료되었다면 아무것도 하지 않습니다.
	if (bIsEnded)
	{
		return;
	}

	if (OwningAction.IsValid())
	{
		FContextPayload ContextPayload;
		OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Delay_Finished, ContextPayload);
	}

	EndTask(true);
}