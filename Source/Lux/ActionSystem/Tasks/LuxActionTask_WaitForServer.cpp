// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask_WaitForServer.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "LuxLogChannels.h"

ULuxActionTask_WaitForServer::ULuxActionTask_WaitForServer()
{
	CheckIntervalSeconds = 0.1f;
	ElapsedTime = 0.0f;
	ActiveCountPtr = nullptr;
}

ULuxActionTask_WaitForServer* ULuxActionTask_WaitForServer::WaitForServer(ULuxAction* InOwningAction, float CheckInterval)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] WaitForServer failed on Owning Action is null."), *GetNameSafe(InOwningAction));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced &&
		"WaitForServer failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_WaitForServer* NewTask = NewObject<ULuxActionTask_WaitForServer>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("WaitForServer failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("WaitForServer");
	NewTask->OwningAction = InOwningAction;
	NewTask->CheckIntervalSeconds = CheckInterval;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_WaitForServer::OnActivated()
{
	Super::OnActivated();

	if (!OwningAction.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] 서버 액션 활성화 대기 실패. OwningAction 이 유효하지 않습니다."),
			ANSI_TO_TCHAR(__FUNCTION__));
		EndTask(true);
		return;
	}

	FLuxActionSpec* Spec = OwningAction->GetLuxActionSpec();
	if (!Spec)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 액션 활성화 대기 실패. Spec 이 유효하지 않습니다."),
			*OwningAction->GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		EndTask(true);
		return;
	}

	ActiveCountPtr = &Spec->ActivationCount;
	ElapsedTime = 0.0f;

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][%s] WaitForServer 태스크 시작. 초기 ActivationCount: %d, IsActive: %s, 체크 간격: %.2f초"),
		*OwningAction->GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), Spec->ActivationCount, 
		Spec->IsActive() ? TEXT("True") : TEXT("False"), CheckIntervalSeconds);

	// 주기적으로 서버 액션 상태를 확인
	GetWorld()->GetTimerManager().SetTimer(
		CheckTimerHandle,
		this,
		&ULuxActionTask_WaitForServer::CheckServerActionStatus,
		CheckIntervalSeconds,
		true
	);
}

void ULuxActionTask_WaitForServer::OnEnded(bool bWasCancelled)
{
	// 타이머 정리
	if (CheckTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	Super::EndTask(bWasCancelled);
}

void ULuxActionTask_WaitForServer::CheckServerActionStatus()
{
	if (!OwningAction.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] 서버 액션 활성화 대기 실패. OwningAction 이 유효하지 않습니다."),
			ANSI_TO_TCHAR(__FUNCTION__));
		EndTask(false);
		return;
	}

	// Spec 재확인
	FLuxActionSpec* Spec = OwningAction->GetLuxActionSpec();
	if (!Spec)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] Spec이 유효하지 않습니다. 태스크를 종료합니다."),
			*OwningAction->GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		EndTask(true);
		return;
	}

	ElapsedTime += CheckIntervalSeconds;

	// 최대 대기 시간 초과 체크
	if (ElapsedTime >= MaxWaitTime)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] 최대 대기 시간 초과 (%.2f초). 태스크 강제 종료. 현재 ActivationCount: %d, IsActive: %s"),
			*OwningAction->GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), MaxWaitTime, 
			ActiveCountPtr ? *ActiveCountPtr : -1, Spec->IsActive() ? TEXT("True") : TEXT("False"));
		EndTask(false);
		return;
	}

	// 현재 상태값들 로깅 (디버깅용)
	UE_LOG(LogLuxActionSystem, VeryVerbose, TEXT("[%s][%s] 서버 액션 상태 확인 중... 경과 시간: %.2f초, ActivationCount: %d, IsActive: %s"),
		*OwningAction->GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), ElapsedTime, 
		ActiveCountPtr ? *ActiveCountPtr : -1, Spec->IsActive() ? TEXT("True") : TEXT("False"));

	// ActivationCount 또는 IsActive() 중 하나라도 활성화 상태를 나타내면 성공
	if ((ActiveCountPtr && *ActiveCountPtr > 0) || Spec->IsActive())
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][%s] 서버 액션이 활성화되었습니다! 경과 시간: %.2f초, ActivationCount: %d, IsActive: %s"),
			*OwningAction->GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), ElapsedTime, 
			ActiveCountPtr ? *ActiveCountPtr : -1, Spec->IsActive() ? TEXT("True") : TEXT("False"));

		// 서버 액션이 활성화되었으면 이벤트 발생
		OnServerActionActive.Broadcast();
		EndTask(false); // 성공적으로 완료
	}
}