// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Tasks/LuxActionTask_PlayMontageAndWait.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"


ULuxActionTask_PlayMontageAndWait* ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(
	ULuxAction* InOwningAction,
	UAnimMontage* MontageToPlay,
	float Rate,
	FName StartSection,
	bool bStopWhenAbilityEnds)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] PlayMontageAndWaitTask failed on Owning Action is null."), *GetNameSafe(InOwningAction));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced &&
		"PlayMontageAndWaitTask failed: Non-Instanced Actions cannot create Tasks.");

	if (!MontageToPlay)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("PlayMontageAndWaitTask failed for Action [%s] on MontageToPlay is null."),
			*InOwningAction->GetName(), *GetNameSafe(MontageToPlay));
		return nullptr;
	}

	ULuxActionTask_PlayMontageAndWait* NewTask = NewObject<ULuxActionTask_PlayMontageAndWait>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("PlayMontageAndWaitTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("PlayMontageAndWait");
	NewTask->OwningAction = InOwningAction;
	NewTask->MontageToPlay = MontageToPlay;
	NewTask->Rate = Rate;
	NewTask->StartSection = StartSection;
	NewTask->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_PlayMontageAndWait::InitializeFromStruct(const FInstancedStruct& Struct)
{
	if (const FPlayMontageAndWaitParams* Params = Struct.GetPtr<FPlayMontageAndWaitParams>())
	{
		MontageToPlay = Params->MontageToPlay;
		Rate = Params->Rate.GetValue(OwningAction.Get());
		StartSection = Params->StartSection;
		bStopWhenAbilityEnds = Params->bStopWhenAbilityEnds;
	}
}

void ULuxActionTask_PlayMontageAndWait::OnActivated()
{
	Super::OnActivated();

	// 플래그들을 초기화합니다.
	bWasSkipped = false;
	bIsBlendingOut = false;
	bWasCompleted = false;

	UActionSystemComponent* ASC = OwningAction->GetActionSystemComponent();
	ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor());
	if (!ASC || !Character)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] OnActivated: ASC 또는 Character가 유효하지 않습니다."), *GetNameSafe(this));
		EndTask(false);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] OnActivated: AnimInstance가 유효하지 않습니다."), *GetNameSafe(this));
		EndTask(false);
		return;
	}

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] OnActivated: '%s' 몽타주 재생 시작"), *GetNameSafe(this), *GetNameSafe(MontageToPlay));

	// 델리게이트를 연결합니다.
	AnimInstance->OnMontageBlendingOut.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageBlendingOut);
	AnimInstance->OnMontageEnded.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageEnded);
	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyBeginReceived);
	AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyEndReceived);

	// 몽타주를 재생합니다.
	if (Character->PlayAnimMontage(MontageToPlay, Rate, StartSection) > 0.f)
	{
		// 성공
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] OnActivated: PlayAnimMontage 실패. 액션을 종료합니다."), *GetNameSafe(this));
		EndTask(false);
	}
}

void ULuxActionTask_PlayMontageAndWait::OnEnded(bool bSuccess)
{
	Super::OnEnded(bSuccess);

	if (!OwningAction.IsValid())
		return;

	UActionSystemComponent* ASC = OwningAction->GetActionSystemComponent();
	if (!ASC) return;

	ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor());
	if (!Character) return;

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	// 델리게이트를 해제합니다.
	AnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageBlendingOut);
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageEnded);
	AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyBeginReceived);
	AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyEndReceived);

	// 태스크 종료 시 몽타주를 중지하도록 설정된 경우
	if (bStopWhenAbilityEnds && AnimInstance->Montage_IsPlaying(MontageToPlay))
	{
		AnimInstance->Montage_Stop(0.20f, MontageToPlay);
	}
}

void ULuxActionTask_PlayMontageAndWait::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsEnded) return;

	if (!OwningAction.IsValid())
		return;

	if (Montage == MontageToPlay)
	{
		bIsBlendingOut = true;
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] OnMontageBlendingOut: 블렌드 아웃 시작"), *GetNameSafe(this));
	}
}

void ULuxActionTask_PlayMontageAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsEnded) return;

	if (!OwningAction.IsValid() || Montage != MontageToPlay)
	{
		return;
	}

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] OnMontageEnded: 몽타주 종료 (중단: %s)"), *GetNameSafe(this), bInterrupted ? TEXT("예") : TEXT("아니오"));

	// 액션이 이미 종료된 상태에서 발생하는 중단 이벤트는 무시
	if (bInterrupted && !OwningAction.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] OnMontageEnded: 액션이 이미 종료된 상태에서 발생하는 중단 이벤트는 무시"), *GetNameSafe(this));
		EndTask(false);
		return;
	}

	// 몽타주가 정상적으로 종료된 경우
	if (!bInterrupted)
	{
		bWasCompleted = true;
		FContextPayload Payload;
		OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Montage_Ended, Payload);
	}
	// 몽타주가 중단된 경우
	else
	{
		FContextPayload Payload;
		OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Montage_Interrupted, Payload);
	}

	// 몽타주가 완전히 종료되면 태스크를 종료합니다.
	EndTask(!bInterrupted);
}

void ULuxActionTask_PlayMontageAndWait::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	// 이미 다른 이유로 태스크가 종료되었다면 아무것도 하지 않습니다.
	if (bIsEnded) return;

	if (!OwningAction.IsValid())
		return;

	FPayload_Name NamePayload;
	NamePayload.Value = NotifyName;

	FContextPayload ContextPayload;
	ContextPayload.SetData(LuxPayloadKeys::NotifyName, NamePayload);

	OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Montage_NotifyBegin, ContextPayload);
}

void ULuxActionTask_PlayMontageAndWait::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	// 이미 다른 이유로 태스크가 종료되었다면 아무것도 하지 않습니다.
	if (bIsEnded) return;

	if (!OwningAction.IsValid())
		return;

	FPayload_Name NamePayload;
	NamePayload.Value = NotifyName;

	FContextPayload ContextPayload;
	ContextPayload.SetData(LuxPayloadKeys::NotifyName, NamePayload);

	OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Montage_NotifyEnd, ContextPayload);
}

void ULuxActionTask_PlayMontageAndWait::UnbindDelegates()
{
	if (!OwningAction.IsValid())
		return;

	UActionSystemComponent* ASC = OwningAction->GetActionSystemComponent();
	if (!ASC) return;

	ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor());
	if (!Character) return;

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	// 델리게이트 해제
	AnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageBlendingOut);
	AnimInstance->OnMontageEnded.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageEnded);
	AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyBeginReceived);
	AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyEndReceived);
}

void ULuxActionTask_PlayMontageAndWait::OnBeforeReHome()
{
	// ReHome 전에 델리게이트를 해제합니다.
	UActionSystemComponent* OldASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
	ACharacter* OldCharacter = OldASC ? Cast<ACharacter>(OldASC->GetAvatarActor()) : nullptr;
	UAnimInstance* OldAnimInstance = OldCharacter ? OldCharacter->GetMesh()->GetAnimInstance() : nullptr;

	if (OldAnimInstance)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] ReHome: 델리게이트를 해제합니다."), *GetNameSafe(this));
		OldAnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageBlendingOut);
		OldAnimInstance->OnMontageEnded.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageEnded);
		OldAnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyBeginReceived);
		OldAnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyEndReceived);
	}
}

void ULuxActionTask_PlayMontageAndWait::OnAfterReHome()
{
	// ReHome 후 델리게이트를 다시 연결합니다.
	UActionSystemComponent* NewASC = OwningAction.IsValid() ? OwningAction->GetActionSystemComponent() : nullptr;
	ACharacter* NewCharacter = NewASC ? Cast<ACharacter>(NewASC->GetAvatarActor()) : nullptr;
	UAnimInstance* NewAnimInstance = NewCharacter ? NewCharacter->GetMesh()->GetAnimInstance() : nullptr;

	if (NewAnimInstance)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] ReHome: 델리게이트를 다시 연결합니다."), *GetNameSafe(this));
		NewAnimInstance->OnMontageBlendingOut.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageBlendingOut);
		NewAnimInstance->OnMontageEnded.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnMontageEnded);
		NewAnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyBeginReceived);
		NewAnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &ULuxActionTask_PlayMontageAndWait::OnNotifyEndReceived);
	}
}