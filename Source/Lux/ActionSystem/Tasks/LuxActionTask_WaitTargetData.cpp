// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask_WaitTargetData.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "Game/LuxPlayerController.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

ULuxActionTask_WaitTargetData* ULuxActionTask_WaitTargetData::WaitTargetData(ULuxAction* InOwningAction)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[WaitTargetData] WaitTargetDataTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"WaitTargetDataTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_WaitTargetData* NewTask = NewObject<ULuxActionTask_WaitTargetData>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("WaitTargetDataTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("WaitTargetData");
	NewTask->OwningAction = InOwningAction;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_WaitTargetData::OnActivated()
{
    Super::OnActivated();

    AActor* Avatar = OwningAction->GetAvatarActor();
    if (!Avatar) return;

    APawn* AvatarPawn = Cast<APawn>(Avatar);
    if (!AvatarPawn) return;

    PlayerController = Cast<ALuxPlayerController>(AvatarPawn->GetController());
    if (PlayerController.IsValid())
    {
        PlayerController->OnTargetAcquired.AddDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetAcquiredCallback);
        PlayerController->OnTargetingCancelled.AddDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetingCancelledCallback);
    }
    else
    {
        EndTask(false);
    }
}

void ULuxActionTask_WaitTargetData::OnEnded(bool bSuccess)
{
    if (PlayerController.IsValid())
    {
        PlayerController->OnTargetAcquired.RemoveDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetAcquiredCallback);
        PlayerController->OnTargetingCancelled.RemoveDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetingCancelledCallback);
    }

    Super::OnEnded(bSuccess);
}

void ULuxActionTask_WaitTargetData::OnBeforeReHome()
{
    if (PlayerController.IsValid())
    {
        PlayerController->OnTargetAcquired.RemoveDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetAcquiredCallback);
        PlayerController->OnTargetingCancelled.RemoveDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetingCancelledCallback);
    }
}

void ULuxActionTask_WaitTargetData::OnAfterReHome()
{
    if (PlayerController.IsValid())
    {
        PlayerController->OnTargetAcquired.AddDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetAcquiredCallback);
        PlayerController->OnTargetingCancelled.AddDynamic(this, &ULuxActionTask_WaitTargetData::OnTargetingCancelledCallback);
    }
}

void ULuxActionTask_WaitTargetData::OnTargetAcquiredCallback(const FHitResult& HitResult)
{
    // 이미 다른 이유로 태스크가 종료되었다면 아무것도 하지 않습니다.
    if (bIsEnded) return;

    if (OwningAction.IsValid())
    {
        FContextPayload TaskPayload;
        TaskPayload.SetData(LuxPayloadKeys::TargetingData, HitResult);
        OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_TargetData_Ready, TaskPayload);
    }
  
    EndTask(true);
}

void ULuxActionTask_WaitTargetData::OnTargetingCancelledCallback()
{
    // 이미 다른 이유로 태스크가 종료되었다면 아무것도 하지 않습니다.
    if (bIsEnded) return;

    if (OwningAction.IsValid())
    {
        FContextPayload TaskPayload;
        OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_TargetData_Cancelled, TaskPayload);
    }

    EndTask(false);
}