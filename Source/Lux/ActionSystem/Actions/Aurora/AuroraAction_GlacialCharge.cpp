// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Actions/Aurora/AuroraAction_GlacialCharge.h"

// Lux 프로젝트 헤더
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Tasks/LuxActionTask_PlayMontageAndWait.h"
#include "ActionSystem/Tasks/LuxActionTask_WaitDelay.h"
#include "ActionSystem/Tasks/LuxActionTask_FollowSpline.h"
#include "ActionSystem/Tasks/LuxActionTask_WaitInputPress.h"
#include "ActionSystem/Tasks/Aurora/LuxActionTask_TraceAndBuildPath.h"
#include "Animations/LuxAnimInstance.h"

#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "Cues/LuxCueTags.h"
#include "Character/LuxCharacter.h"
#include "Character/LuxHeroCharacter.h"
#include "Camera/LuxCameraComponent.h"
#include "Actors/Action/Aurora/GlacialPath.h"

UAuroraAction_GlacialCharge::UAuroraAction_GlacialCharge()
{
	InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerExecution;
	ActivationPolicy = ELuxActionActivationPolicy::OnInputTriggered;

	ActionIdentifierTag = LuxActionTags::Action_Aurora_GlacialCharge;

	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Normal);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Movement);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Damaging);

	ReplicatedEventTags.AddTag(LuxGameplayTags::Task_Event_Input_Pressed);

	static ConstructorHelpers::FObjectFinder<UDataTable> LevelDataFinder(TEXT("/Game/Characters/Aurora/DT_Aurora.DT_Aurora"));
	if (LevelDataFinder.Succeeded()) LevelDataTable = LevelDataFinder.Object;
	else LevelDataTable = nullptr;

	/** =================== 'Begin' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'StartDash' 노티파이 발생 시 'Dash' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		Transition.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_Dash;

		// 조건 1: Notify 이름이 'StartDash'인 경우
		FInstancedStruct ConditionNotify = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		ConditionNotify.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartDash");
		Transition.Conditions.Add(ConditionNotify);

		// 예외 1: 경로 찾기 실패 시 'Interrupt' 페이즈로 전환
		FPhaseTransition PathFailureTransition;
		PathFailureTransition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		PathFailureTransition.EventTag = LuxGameplayTags::Task_Event_Path_Failed;
		PathFailureTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Interrupt;

		// 예외: 시간 초과 시 'Interrupt' 페이즈로 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 3.0f; // 3초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Interrupt;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(PathFailureTransition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(TimeoutTransition);
	}

	/** =================== 'Dash' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'StartWaitRecast' 노티파이 발생 시 'WaitRecast' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		Transition.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_WaitRecast;

		// 조건 1: Notify 이름이 'StartWaitRecast'인 경우
		FInstancedStruct ConditionNotify = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		ConditionNotify.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartWaitRecast");
		Transition.Conditions.Add(ConditionNotify);

		// 예외: 시간 초과 시 'WaitRecast' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 2.0f; // 2초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_WaitRecast;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Dash).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Dash).Add(TimeoutTransition);
	}

	/** =================== 'WaitRecast' 페이즈 전환 규칙 =================== */
	{
		// 성공 1: 재입력 시 'Recast' 페이즈로 전환
		FPhaseTransition RecastTransition;
		RecastTransition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		RecastTransition.EventTag = LuxGameplayTags::Task_Event_Input_Pressed;
		RecastTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Recast;

		// 성공 2: 시간 초과 시 'End' 페이즈로 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		TimeoutTransition.EventTag = LuxGameplayTags::Task_Event_Delay_Finished;
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_WaitRecast).Add(RecastTransition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_WaitRecast).Add(TimeoutTransition);
	}

	/** =================== 'DestroyPath' 페이즈 전환 규칙 =================== */
	{
		// 경로 파괴 후 즉시 'End' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::Immediate;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recast).Add(Transition);
	}

	/** =================== 'Interrupt' 페이즈 전환 규칙 =================== */
	{
		// 즉시 'End' 페이즈로 넘어가서 액션을 종료시킴
		FPhaseTransition ImmediateEndTransition;
		ImmediateEndTransition.TransitionType = EPhaseTransitionType::Immediate;
		ImmediateEndTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Interrupt).Add(ImmediateEndTransition);
	}

}

void UAuroraAction_GlacialCharge::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
	Super::OnPhaseEnter(PhaseTag, SourceASC);

	if (PhaseTag == LuxPhaseTags::Phase_Action_Begin)				PhaseAnalyze(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Dash)			PhaseDash(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_WaitRecast)		PhaseWaitRecast(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Recast)			PhaseDestroyPath(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Interrupt)		PhaseInterrupt(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_End)			PhaseEnd(SourceASC);
}

void UAuroraAction_GlacialCharge::OnActionEnd(bool bIsCancelled)
{
	if (bIsCancelled && ::IsValid(GlacialPathActor))
	{
		GlacialPathActor->Stop();
	}

	FGameplayTagContainer TagsToUnblock;
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Rotation);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Action);

	RemoveTags(TagsToUnblock, 1);

	UActionSystemComponent* SourceASC = GetActionSystemComponent();
	if (SourceASC)
	{
		// 매크로로 구독한 이벤트 수동 해제
		UNSUBSCRIBE_FROM_GAMEPLAY_EVENT(LuxGameplayTags::Event_Movement_Started, HandleGameplayEventForInterruption);
		SourceASC->StopGameplayCue(GetAvatarActor(), LuxCueTags::Cue_Aurora_GlacialCharge_FrostShield);
	}

	if (bIsCameraModePushed)
	{
		if (ALuxHeroCharacter* Hero = Cast<ALuxHeroCharacter>(GetAvatarActor()))
		{
			if (ULuxCameraComponent* CameraComponent = Hero->GetCameraComponent())
			{
				CameraComponent->PopCameraMode();
			}
		}
	}

	Super::OnActionEnd(bIsCancelled);
}

void UAuroraAction_GlacialCharge::PhaseAnalyze(UActionSystemComponent& SourceASC)
{
	AActor* AvatarActor = SourceASC.GetAvatarActor();
	if (!AvatarActor)
	{
		return;
	}

	const int32 CurrentLevel = GetActionLevel();
	const FName RowName = FName(*FString::FromInt(CurrentLevel));
	const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("Phase.Analyze.TaskParams"));

	if (!LevelData)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("LevelDataTable에서 레벨 %d 데이터를 찾을 수 없어 액션을 종료합니다."), CurrentLevel);
		EndAction();
		return;
	}

	const FAuroraActionLevelData_GlacialCharge* GlacialChargetData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_GlacialCharge>();
	if (!GlacialChargetData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("Level '%d' 의 LevelData에서 FAuroraActionLevelData_GlacialCharge 타입의 데이터를 찾을 수 없습니다. "), CurrentLevel);
		return;
	}

	// 서버에서만 TraceAndBuildPath 태스크를 생성하여 캐릭터 앞 지형을 분석하여 경로를 생성합니다.
	if (AvatarActor->HasAuthority())
	{
		SUBSCRIBE_TO_TASK_EVENT(LuxGameplayTags::Task_Event_Path_Ready, OnPathReady);
		ULuxActionTask_TraceAndBuildPath* PathTask = ULuxActionTask_TraceAndBuildPath::TraceAndBuildPath(this, GlacialChargetData->Range);
	}

	// 서버와 클라이언트 모두 애니메이션을 재생합니다. (예측 시스템을 위해)
	ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(this, MontageToPlay, 1.0f);

	// 움직임을 차단합니다. (서버에서만 실행 됩니다)
	FGameplayTagContainer TagsToBlock;
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Rotation);
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Action);

	AddTags(TagsToBlock, 1);
}

void UAuroraAction_GlacialCharge::OnPathReady(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	const FPayload_PathData* PathData = Payload.GetData<FPayload_PathData>(LuxPayloadKeys::PathData);
	if (PathData)
	{
		ActionPayload->SetData(LuxPayloadKeys::PathData, *PathData);
		UE_LOG(LogLuxActionSystem, Log, TEXT("Action [%s]: 경로 데이터를 성공적으로 수신했습니다. 포인트 개수: %d"), *GetName(), PathData->PathPoints.Num());
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("Action [%s]: OnPathReady가 호출되었으나 페이로드에서 경로 데이터를 찾을 수 없습니다."), *GetName());
	}
}

void UAuroraAction_GlacialCharge::PhaseDash(UActionSystemComponent& SourceASC)
{
	APawn* AvatarPawn = Cast<APawn>(SourceASC.GetAvatarActor());
	if (!AvatarPawn) return;

	if (AvatarPawn->IsLocallyControlled())
	{
		// 로컬 클라이언트에서 돌진 연출을 위한 카메라 모드를 활성화합니다.
		//DashClient(SourceASC);
	}
	else if (AvatarPawn->HasAuthority())
	{
		// 서버에서 캐릭터의 실제 이동과 액터 생성을 처리합니다.
		DashServer(SourceASC);
	}
}

void UAuroraAction_GlacialCharge::DashClient(UActionSystemComponent& SourceASC)
{
	// 데이터 에셋으로 이전하였습니다...

	// 돌진 연출에 사용할 전용 카메라 모드를 활성화합니다.
	/*ALuxHeroCharacter* Hero = Cast<ALuxHeroCharacter>(GetAvatarActor());
	if (!Hero) return;

	if (ULuxCameraComponent* CameraComponent = Hero->GetCameraComponent())
	{
		CameraComponent->PushCameraMode(DashCameraMode);
		bIsCameraModePushed = true;
	}*/
}

void UAuroraAction_GlacialCharge::DashServer(UActionSystemComponent& SourceASC)
{
	// 캐릭터의 실제 이동과 액터 생성은 서버에서만 처리합니다.
	AActor* AvatarActor = SourceASC.GetAvatarActor();
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}

	// 현재 스킬 레벨에 맞는 데이터를 데이터 테이블에서 가져옵니다.
	if (!LevelDataTable)
	{
		EndAction();
		return;
	}

	const int32 CurrentLevel = GetActionLevel();
	const FName RowName = FName(*FString::FromInt(CurrentLevel));
	const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("Phase.Dash"));
	if (!LevelData)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("LevelDataTable에서 레벨 %d 데이터를 찾을 수 없어 액션을 종료합니다."), CurrentLevel);
		EndAction();
		return;
	}

	const FAuroraActionLevelData_GlacialCharge* GlacialPathtData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_GlacialCharge>();
	if (!GlacialPathtData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("Level '%d' 의 LevelData에서 FAuroraActionLevelData_GlacialCharge 타입의 데이터를 찾을 수 없습니다."), CurrentLevel);
		EndAction();
		return;
	}

	// 액션이 캐릭터의 움직임을 직접 제어하도록 상태를 변경합니다.
	ALuxCharacter* Character = Cast<ALuxCharacter>(AvatarActor);
	if (Character)
	{
		Character->SetActionControlledMovement(true);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwnerActor();
	SpawnParams.Instigator = AvatarActor->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector OriginLocation = AvatarActor->GetActorLocation() - FVector(0.f, 0.f, 95.f);
	FRotator OrginRotation = AvatarActor->GetActorRotation();

	// 이전 'Analyze' 페이즈에서 생성된 경로 데이터를 가져옵니다.
	const FPayload_PathData* PathData = ActionPayload->GetData<FPayload_PathData>(FName("PathData"));
	if (!PathData || PathData->PathPoints.Num() < 2)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("PhaseDash 실패: ActionPayload에서 유효한 경로 데이터를 찾을 수 없습니다."));
		EndAction();
		return;
	}

	// 얼음 길을 시각적으로 표현할 GlacialPath 액터를 월드에 생성합니다.
	GlacialPathActor = GetWorld()->SpawnActor<AGlacialPath>(GlacialPathClass, OriginLocation, OrginRotation, SpawnParams);
	if (GlacialPathActor)
	{
		GlacialPathActor->Initialize(this);
	}

	// 캐릭터가 생성된 얼음 길을 따라 이동하도록 태스크를 시작합니다.
	USplineComponent* PathSpline = GlacialPathActor->GetSplineComponent();
	ULuxActionTask_FollowSpline::FollowSpline(this, PathSpline, GlacialPathtData->CreationDuration);

	// 돌진 시 캐릭터 몸에 표시될 방어막 같은 시각 효과(Cue)를 모든 클라이언트에게 재생하도록 요청합니다.
	FLuxCueContext Context;
	Context.Instigator = AvatarActor;
	Context.Location = AvatarActor->GetActorLocation();
	Context.Rotation = AvatarActor->GetActorRotation();
	Context.EffectCauser = GetOwnerActor();
	Context.SourceASC = &SourceASC;
	Context.Level = CurrentLevel;
	Context.CueTags.AddTag(LuxGameplayTags::CrowdControl_Snare);
	SourceASC.ExecuteGameplayCue(AvatarActor, LuxCueTags::Cue_Aurora_GlacialCharge_FrostShield, Context);
}


void UAuroraAction_GlacialCharge::PhaseWaitRecast(UActionSystemComponent& SourceASC)
{
	APawn* AvatarPawn = Cast<APawn>(SourceASC.GetAvatarActor());
	if (!AvatarPawn) return;

	const FActiveLuxAction* ActiveAction = SourceASC.FindActiveAction(GetActiveHandle());
	if (!ActiveAction || !LevelDataTable)
	{
		EndAction();
		return;
	}

	// 데이터 테이블에서 재시전 대기 시간 조회
	const int32 CurrentLevel = ActiveAction->Spec.Level;
	const FName RowName = FName(*FString::FromInt(CurrentLevel));
	const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("Phase.WaitRecast"));
	if (!LevelData)
	{
		EndAction();
		return;
	}

	const FAuroraActionLevelData_GlacialCharge* GlacialChargeData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_GlacialCharge>();
	if (!GlacialChargeData)
	{
		EndAction();
		return;
	}

	ALuxCharacter* Character = Cast<ALuxCharacter>(AvatarPawn);
	if (Character)
	{
		Character->SetActionControlledMovement(false);
	}

	if (AvatarPawn->IsLocallyControlled())
	{
		if (ALuxHeroCharacter* Hero = Cast<ALuxHeroCharacter>(GetAvatarActor()))
		{
			if (ULuxCameraComponent* CameraComponent = Hero->GetCameraComponent())
			{
				CameraComponent->PopCameraMode();
				bIsCameraModePushed = false;
			}
		}
	}

	// 시간 초과 타이머 시작
	ULuxActionTask_WaitDelay::WaitDelay(this, GlacialChargeData->PathLifetime);

	// 재입력 대기 태스크 시작 (현재 액션의 입력 태그를 사용)
	ULuxActionTask_WaitInputPress::WaitInputPress(this, ActiveAction->Spec.InputTag);

	SUBSCRIBE_TO_GAMEPLAY_EVENT(LuxGameplayTags::Event_Movement_Started, HandleGameplayEventForInterruption);

	FGameplayTagContainer TagsToUnblock;
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Rotation);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Action);

	RemoveTags(TagsToUnblock, 1);
	SourceASC.StopGameplayCue(GetAvatarActor(), LuxCueTags::Cue_Aurora_GlacialCharge_FrostShield);
}

void UAuroraAction_GlacialCharge::PhaseDestroyPath(UActionSystemComponent& SourceASC)
{
	if (GlacialPathActor)
	{
		GlacialPathActor->Stop();
		GlacialPathActor = nullptr;
	}
}

void UAuroraAction_GlacialCharge::PhaseInterrupt(UActionSystemComponent& SourceASC)
{
	// 현재 재생 중인 몽타주 태스크를 찾아 중단시킵니다.
	for (ULuxActionTask* ActiveTask : ActiveTasks)
	{
		if (ULuxActionTask_PlayMontageAndWait* MontageTask = Cast<ULuxActionTask_PlayMontageAndWait>(ActiveTask))
		{
			MontageTask->EndTask(false);
			break;
		}
	}
}

void UAuroraAction_GlacialCharge::PhaseEnd(UActionSystemComponent& SourceASC)
{
	EndAction();
}