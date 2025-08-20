// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask_WaitInputRelease.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

ULuxActionTask_WaitInputRelease* ULuxActionTask_WaitInputRelease::WaitInputRelease(ULuxAction* InOwningAction, FGameplayTag InTagToWaitFor)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[WaitInputRelease] WaitInputReleaseTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"WaitInputReleaseTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_WaitInputRelease* NewTask = NewObject<ULuxActionTask_WaitInputRelease>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("WaitInputReleaseTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("WaitInputRelease");
	NewTask->OwningAction = InOwningAction;
	NewTask->TagToWaitFor = InTagToWaitFor;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_WaitInputRelease::InitializeFromStruct(const FInstancedStruct& Struct)
{
    if (const FWaitInputReleaseParams* Params = Struct.GetPtr<FWaitInputReleaseParams>())
    {
        TagToWaitFor = Params->InputTagToWait;
    }
}

void ULuxActionTask_WaitInputRelease::OnActivated()
{
    Super::OnActivated();

    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        ASC->OnLocalInputTagReleased.AddDynamic(this, &ULuxActionTask_WaitInputRelease::OnInputRelease);
    }
}

void ULuxActionTask_WaitInputRelease::OnEnded(bool bSuccess)
{
    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        ASC->OnLocalInputTagReleased.RemoveDynamic(this, &ULuxActionTask_WaitInputRelease::OnInputRelease);
    }

    Super::OnEnded(bSuccess);
}

void ULuxActionTask_WaitInputRelease::OnBeforeReHome()
{
    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        ASC->OnLocalInputTagReleased.RemoveDynamic(this, &ULuxActionTask_WaitInputRelease::OnInputRelease);
    }
}

void ULuxActionTask_WaitInputRelease::OnAfterReHome()
{
    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        ASC->OnLocalInputTagReleased.AddDynamic(this, &ULuxActionTask_WaitInputRelease::OnInputRelease);
    }
}

void ULuxActionTask_WaitInputRelease::OnInputRelease(const FGameplayTag& InputTag)
{
    if (bIsEnded) return;

    if (bWasReleased)  return;
    bWasReleased = true;

    if (TagToWaitFor.IsValid() && InputTag.MatchesTagExact(TagToWaitFor) == false)
    {
        return;
    }

    FContextPayload TaskPayload;
    OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Input_Released, TaskPayload);

    EndTask(true);
}