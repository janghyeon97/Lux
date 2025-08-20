// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Tasks/LuxActionTask_WaitPhaseDelay.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"


ULuxActionTask_WaitPhaseDelay* ULuxActionTask_WaitPhaseDelay::WaitPhaseDelay(ULuxAction* InOwningAction, float WaitDuration)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[WaitPhaseDelay] WaitPhaseDelayTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"WaitPhaseDelayTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_WaitPhaseDelay* NewTask = NewObject<ULuxActionTask_WaitPhaseDelay>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("WaitPhaseDelayTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("WaitPhaseDelay");
	NewTask->OwningAction = InOwningAction;
	NewTask->Duration = WaitDuration;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_WaitPhaseDelay::OnActivated()
{
    Super::OnActivated();

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().SetTimer(TimerHandle, this, &ULuxActionTask_WaitPhaseDelay::OnTimeExpired, Duration, false, Duration);
	}
}

void ULuxActionTask_WaitPhaseDelay::OnEnded(bool bSuccess)
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(TimerHandle);
    }

	Super::OnEnded(bSuccess);
}

void ULuxActionTask_WaitPhaseDelay::OnTimeExpired()
{
    if (bIsEnded) return;

    if (OwningAction.IsValid())
    {
        FContextPayload Payload;
        OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_PhaseDelay_Finished, Payload);
    }

    EndTask(true);
}