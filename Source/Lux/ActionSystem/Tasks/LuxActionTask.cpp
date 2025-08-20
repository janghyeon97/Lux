// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

class FLifetimeProperty;

ULuxActionTask::ULuxActionTask()
{
	bIsEnded = false;
	OwningAction = nullptr;
	LifecycleState = ELuxTaskLifecycleState::Inactive;
}

void ULuxActionTask::Activate()
{
	if (LifecycleState != ELuxTaskLifecycleState::Inactive)
	{
		UE_LOG(LogLux, Warning, TEXT("Task %s is already active or ended. Current state: %d"), *TaskName.ToString(), static_cast<int32>(LifecycleState));
		return;
	}

	LifecycleState = ELuxTaskLifecycleState::Executing;

	if (OwningAction.IsValid())
	{
		OwningAction->OnTaskActivated(this);
	}

	OnActivated();
}

void ULuxActionTask::EndTask(bool bSuccess)
{
	if (LifecycleState == ELuxTaskLifecycleState::Ended)
	{
		UE_LOG(LogLux, Warning, TEXT("Task %s is already ended."), *TaskName.ToString());
		return;
	}

	if (bIsEnded.AtomicSet(true) == false)
	{
		LifecycleState = ELuxTaskLifecycleState::Ending;
		OnEnded(bSuccess);
		LifecycleState = ELuxTaskLifecycleState::Ended;

		if (OwningAction.IsValid())
		{
			OwningAction->OnTaskEnded(this);
		}
	}
}

void ULuxActionTask::ReHome(ULuxAction* NewOwningAction)
{
	if (!NewOwningAction)
	{
		return;
	}

	OnBeforeReHome();
	Rename(nullptr, NewOwningAction);
	OwningAction = NewOwningAction;
	OnAfterReHome();
}
 
void ULuxActionTask::OnActivated()
{
	// 자식 클래스에서 재정의합니다.
}

void ULuxActionTask::OnEnded(bool bSuccess)
{
	// 자식 클래스에서 재정의합니다.
}

void ULuxActionTask::OnBeforeReHome()
{

}

void ULuxActionTask::OnAfterReHome()
{

}