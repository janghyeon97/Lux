// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask_WaitInputPress.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

ULuxActionTask_WaitInputPress* ULuxActionTask_WaitInputPress::WaitInputPress(ULuxAction* InOwningAction, FGameplayTag InTagToWaitFor)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[WaitInputPress] WaitInputPressTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"WaitInputPressTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_WaitInputPress* NewTask = NewObject<ULuxActionTask_WaitInputPress>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("WaitInputPressTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("WaitInputPress");
	NewTask->OwningAction = InOwningAction;
	NewTask->TagToWaitFor = InTagToWaitFor;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_WaitInputPress::InitializeFromStruct(const FInstancedStruct& Struct)
{
    if (const FWaitInputPressParams* Params = Struct.GetPtr<FWaitInputPressParams>())
    {
        TagToWaitFor = Params->InputTagToWait;
    }
}

void ULuxActionTask_WaitInputPress::OnActivated()
{
    Super::OnActivated();

    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        bWasPressed = false;
        ASC->OnLocalInputTagPressed.AddDynamic(this, &ULuxActionTask_WaitInputPress::OnInputPress);
    }
}

void ULuxActionTask_WaitInputPress::OnEnded(bool bSuccess)
{
    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        ASC->OnLocalInputTagPressed.RemoveDynamic(this, &ULuxActionTask_WaitInputPress::OnInputPress);
    }

    Super::OnEnded(bSuccess);
}

void ULuxActionTask_WaitInputPress::OnBeforeReHome()
{
    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        ASC->OnLocalInputTagPressed.RemoveDynamic(this, &ULuxActionTask_WaitInputPress::OnInputPress);
    }
}

void ULuxActionTask_WaitInputPress::OnAfterReHome()
{
    UActionSystemComponent* ASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
    if (ASC)
    {
        ASC->OnLocalInputTagPressed.AddDynamic(this, &ULuxActionTask_WaitInputPress::OnInputPress);
    }
}

void ULuxActionTask_WaitInputPress::OnInputPress(const FGameplayTag& InputTag)
{
    if (bIsEnded) return;

    if (bWasPressed)  return;
    bWasPressed = true;

    // 들어온 입력 태그가 우리가 기다리던 태그와 정확히 일치하는지 확인합니다.
    if (TagToWaitFor.IsValid() && !InputTag.MatchesTagExact(TagToWaitFor))
    {
        return;
    }

    FContextPayload TaskPayload;
    OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Input_Pressed, TaskPayload);
    EndTask(true);
}