// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Actions/Aurora/AuroraAction_Hoarfrost.h"

// Unreal Engine 헤더
#include "Engine/World.h"

// Lux 프로젝트 헤더
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Tasks/LuxActionTask_WaitDelay.h"
#include "ActionSystem/Tasks/LuxActionTask_PlayMontageAndWait.h"

#include "Actors/Action/Aurora/Hoarfrost.h"
#include "Cues/LuxCueTags.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"


UAuroraAction_Hoarfrost::UAuroraAction_Hoarfrost()
{
	InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerExecution;
	ActivationPolicy = ELuxActionActivationPolicy::OnInputTriggered;

	ActionIdentifierTag = LuxActionTags::Action_Aurora_Hoarfrost;

	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Normal);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_CrowdControl);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Damaging);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Debuff);

	static ConstructorHelpers::FObjectFinder<UDataTable> LevelDataFinder(TEXT("/Game/Characters/Aurora/DT_Aurora.DT_Aurora"));
	if (LevelDataFinder.Succeeded()) LevelDataTable = LevelDataFinder.Object;
	else LevelDataTable = nullptr;


	/** =================== 'Begin' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'StartFreeze' 노티파이 발생 시 'Execute' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		Transition.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_Execute;

		// 조건: Notify 이름이 'StartFreeze'인 경우
		FInstancedStruct Condition = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		Condition.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartFreeze");
		Transition.Conditions.Add(Condition);

		// 예외: 시간 초과 시 'Execute' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 1.5f; // 1.5초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Execute;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(TimeoutTransition);
	}

	/** =================== 'Execute' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'StartRecovery' 노티파이 발생 시 'Recovery' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		Transition.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_Recovery;

		// 조건: Notify 이름이 'StartRecovery'인 경우
		FInstancedStruct Condition = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		Condition.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartRecovery");
		Transition.Conditions.Add(Condition);

		// 예외: 시간 초과 시 'Recovery' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 1.0f; // 1초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Recovery;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Execute).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Execute).Add(TimeoutTransition);
	}

	/** =================== 'Recovery' 페이즈 전환 규칙 =================== */
	{
		// 성공: 몽타주가 정상적으로 끝나면 'End' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		Transition.EventTag = LuxGameplayTags::Task_Event_Montage_Ended;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		// 캔슬: 이동 입력이 들어오면 'Interrupt' 페이즈로 전환
		FPhaseTransition MovementInterruptTransition;
		MovementInterruptTransition.TransitionType = EPhaseTransitionType::OnGameplayEvent;
		MovementInterruptTransition.EventTag = LuxGameplayTags::Event_Movement_Started;
		MovementInterruptTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Interrupt;

		// 예외: 시간 초과 시 'End' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 1.5f; // 1.5초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(MovementInterruptTransition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(TimeoutTransition);
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

void UAuroraAction_Hoarfrost::OnActionEnd(bool bIsCancelled)
{
	if (HoarfrostActor && bIsCancelled)
	{
		HoarfrostActor->Stop();
	}

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (ASC)
	{
		RemoveTag(LuxGameplayTags::State_Block_Movement, 1);
		RemoveTag(LuxGameplayTags::State_Block_Action, 1);

		if (bIsCancelled)
		{
			ASC->StopGameplayCue(GetAvatarActor(), LuxCueTags::Cue_Aurora_Hoarfrost_FreezeSegments);
		}
	}

	Super::OnActionEnd(bIsCancelled);
}


void UAuroraAction_Hoarfrost::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
	Super::OnPhaseEnter(PhaseTag, SourceASC);

	if		(PhaseTag == LuxPhaseTags::Phase_Action_Begin)			PhaseBegin(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Execute)		PhaseFreeze(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Recovery)		PhaseRecovery(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Interrupt)		PhaseInterrupt(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_End)			PhaseEnd(SourceASC);
}

void UAuroraAction_Hoarfrost::PhaseBegin(UActionSystemComponent& SourceASC)
{
	ULuxActionTask_PlayMontageAndWait* MontageTask = ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(
		this,
		MontageToPlay,
		1.0f,
		NAME_None
	);

	if (!MontageTask)
	{
		CancelAction();
		return;
	}

	// 애니메이션 재생 시작과 동시에 특정 시간 동안 움직임 입력을 차단합니다.
	FGameplayTagContainer TagsToBlock;
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Action);

	AddTags(TagsToBlock, 1);
}

void UAuroraAction_Hoarfrost::PhaseFreeze(UActionSystemComponent& SourceASC)
{
	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("'%s' 실행에 필요한 AvatarActor 이 없습니다. 액션을 종료합니다."), *GetName());
		EndAction();
		return;
	}

	if (AvatarActor->GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
	{
		FreezeClient(SourceASC);
	}
	else if (AvatarActor->GetLocalRole() == ENetRole::ROLE_Authority)
	{
		FreezeServer(SourceASC);
	}
}

void UAuroraAction_Hoarfrost::FreezeClient(UActionSystemComponent& SourceASC)
{
	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("'%s'에서 유효한 AvatarActor를 찾을 수 없습니다."), *GetName());
		return;
	}

	if (!LevelDataTable)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 LevelDataTable이 지정되지 않았습니다."), *GetName());
		return;
	}

	// 데이터 테이블에서 현재 레벨에 맞는 데이터를 조회합니다.
	const int32 CurrentLevel = GetActionLevel();
	const FName RowName = FName(*FString::FromInt(CurrentLevel));
	FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("LevelBasedDurationExecution"));
	if (!LevelData)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("LevelDataTable에서 Level '%d'에 해당하는 데이터를 찾을 수 없습니다."), CurrentLevel);
		return;
	}

	const FAuroraActionLevelData_Hoarfrost* HoarfrostData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_Hoarfrost>();
	if (!HoarfrostData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("Level '%d' 의 LevelData에서 FAuroraActionLevelData_Hoarfrost 타입의 데이터를 찾을 수 없습니다. "), CurrentLevel);
		return;
	}

	FLuxCueContext Context;
	Context.Instigator = AvatarActor;
	Context.Location = AvatarActor->GetActorLocation();
	Context.Rotation = AvatarActor->GetActorRotation();

	Context.EffectCauser = GetOwnerActor();
	Context.SourceASC = &SourceASC;

	Context.Magnitude = HoarfrostData->Radius;
	Context.Lifetime = HoarfrostData->Duration;
	Context.Rate = HoarfrostData->Rate;
	Context.Count = HoarfrostData->Count;
	Context.Level = CurrentLevel;
	Context.CueTags.AddTag(LuxGameplayTags::CrowdControl_Snare);

	SourceASC.ExecuteGameplayCue(AvatarActor, LuxCueTags::Cue_Aurora_Hoarfrost_FreezeSegments, Context);
}

void UAuroraAction_Hoarfrost::FreezeServer(UActionSystemComponent& SourceASC)
{
	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor) return;

	if (!HoarfrostClass)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 HoarfrostClass가 지정되지 않았습니다. 로드를 시도합니다."), *GetName());
		FName Path = TEXT("/Game/Characters/Aurora/AuroraHoarfrost.AuroraHoarfrost_C");
		HoarfrostClass = LoadClass<AHoarfrost>(nullptr, *Path.ToString());
	}

	if (!HoarfrostClass)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 HoarfrostClass를 로드할 수 없습니다. 액션을 종료합니다."), *GetName());
		EndAction();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAction();
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwnerActor();
	SpawnParams.Instigator = AvatarActor->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector OriginLocation = AvatarActor->GetActorLocation() - FVector(0.f, 0.f, 95.f);
	FRotator OrginRotation = AvatarActor->GetActorRotation();

	HoarfrostActor = World->SpawnActor<AHoarfrost>(
		HoarfrostClass,
		OriginLocation,
		OrginRotation,
		SpawnParams
	);

	if (HoarfrostActor)
	{
		HoarfrostActor->Initialize(this, true);
	}
}

void UAuroraAction_Hoarfrost::PhaseRecovery(UActionSystemComponent& SourceASC)
{
	FGameplayTagContainer TagsToUnblock;
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Action);

	RemoveTags(TagsToUnblock, 1);
}

void UAuroraAction_Hoarfrost::PhaseInterrupt(UActionSystemComponent& SourceASC)
{
	for (ULuxActionTask* ActiveTask : ActiveTasks)
	{
		if (ULuxActionTask_PlayMontageAndWait* FoundMontageTask = Cast<ULuxActionTask_PlayMontageAndWait>(ActiveTask))
		{
			FoundMontageTask->EndTask(false); // false = 중단됨
			break;
		}
	}
}

void UAuroraAction_Hoarfrost::PhaseEnd(UActionSystemComponent& SourceASC)
{
	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] PhaseEnd: End 페이즈에 진입하여 액션을 종료합니다."), *GetName());
	EndAction();
}