// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Actions/Aurora/AuroraAction_FrozenSword.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/ActionSystemGlobals.h"
#include "ActionSystem/Tasks/LuxActionTask_PlayMontageAndWait.h"
#include "ActionSystem/Tasks/LuxActionTask_WaitInputPress.h"
#include "ActionSystem/Tasks/LuxActionTask_WaitForServer.h"
#include "ActionSystem/Effects/LuxEffect.h"
#include "ActionSystem/Attributes/CombatSet.h"

#include "Cues/LuxCueTags.h"

#include "Targeting/LuxTargetingInterface.h"
#include "Targeting/TargetingComponent.h"
#include "System/LuxCombatManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Teams/LuxTeamStatics.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

UAuroraAction_FrozenSword::UAuroraAction_FrozenSword()
{
	InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerActor;
	ActivationPolicy = ELuxActionActivationPolicy::OnInputTriggered;

	ActionIdentifierTag = LuxActionTags::Action_Aurora_FrozenSword;
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Attack);

	ComboCountTag = LuxGameplayTags::Action_Combo_Primary;

	ReplicatedEventTags.AddTag(LuxGameplayTags::Task_Event_Attack_HitPredicted);

	/** =================== 'Begin' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'HitCheck' 노티파이 발생 시 'ProcessHit' 페이즈로 전환
		FPhaseTransition ToProcessHit;
		ToProcessHit.TransitionType = EPhaseTransitionType::OnTaskEvent;
		ToProcessHit.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		ToProcessHit.NextPhaseTag = LuxPhaseTags::Phase_Action_ProcessHit;

		FInstancedStruct ToProcessHitCondition = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		ToProcessHitCondition.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("HitCheck");
		ToProcessHit.Conditions.Add(ToProcessHitCondition);

		// 실패: 'StartRecovery' 노티파이 발생 시 'Recovery' 페이즈로 전환
		FPhaseTransition ToRecovery;
		ToRecovery.TransitionType = EPhaseTransitionType::OnTaskEvent;
		ToRecovery.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		ToRecovery.NextPhaseTag = LuxPhaseTags::Phase_Action_Recovery;

		FInstancedStruct ToRecoveryCondition = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		ToRecoveryCondition.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartRecovery");
		ToRecovery.Conditions.Add(ToRecoveryCondition);

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(ToProcessHit);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(ToRecovery);
	}

	/** =================== 'ProcessHit' (서버 전용) 페이즈 전환 규칙 =================== */
	{
		// 성공: 'StartRecovery' 노티파이 발생 시 'Recovery' 페이즈로 전환
		FPhaseTransition ToRecovery;
		ToRecovery.TransitionType = EPhaseTransitionType::OnTaskEvent;
		ToRecovery.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		ToRecovery.NextPhaseTag = LuxPhaseTags::Phase_Action_Recovery;

		// 조건: Notify 이름이 'StartRecovery'인 경우
		FInstancedStruct Condition = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		Condition.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartRecovery");
		ToRecovery.Conditions.Add(Condition);

		// 예외: 시간 초과 시 'Recovery' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 2.0f; // 2초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Recovery;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_ProcessHit).Add(ToRecovery);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_ProcessHit).Add(TimeoutTransition);
	}

	/** =================== 'Recovery' 페이즈 전환 규칙 =================== */
	{
		// 성공: 몽타주가 정상적으로 종료되면 'End' 페이즈로 전환
		FPhaseTransition OnMontageEnded;
		OnMontageEnded.TransitionType = EPhaseTransitionType::OnTaskEvent;
		OnMontageEnded.EventTag = LuxGameplayTags::Task_Event_Montage_Ended;
		OnMontageEnded.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		// 캔슬: 이동 입력이 들어오면 'Interrupt' 페이즈로 전환
		FPhaseTransition MovementInterrupt;
		MovementInterrupt.TransitionType = EPhaseTransitionType::OnGameplayEvent;
		MovementInterrupt.EventTag = LuxGameplayTags::Event_Movement_Started;
		MovementInterrupt.NextPhaseTag = LuxPhaseTags::Phase_Action_Interrupt;

		// 예외: 시간 초과 시 'End' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 1.5f; // 1.5초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(OnMontageEnded);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(MovementInterrupt);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(TimeoutTransition);
	}

	/** =================== 'Interrupt' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'End' 페이즈로 즉시 전환
		FPhaseTransition ImmediateEnd;
		ImmediateEnd.TransitionType = EPhaseTransitionType::Immediate;
		ImmediateEnd.NextPhaseTag = LuxPhaseTags::Phase_Action_End;
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Interrupt).Add(ImmediateEnd);
	}
}

void UAuroraAction_FrozenSword::OnActionEnd(bool bIsCancelled)
{
	// 서버에서만 구독을 해제하도록 확인합니다.
	if (GetAvatarActor() && GetAvatarActor()->HasAuthority())
	{
		FScriptDelegate DelegateToUnbind;
		DelegateToUnbind.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UAuroraAction_FrozenSword, Server_OnHitPredicted));
		UnsubscribeFromTaskEvent(LuxGameplayTags::Task_Event_Attack_HitPredicted, DelegateToUnbind);
	}

	Super::OnActionEnd(bIsCancelled);
}


void UAuroraAction_FrozenSword::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
	Super::OnPhaseEnter(PhaseTag, SourceASC);

	if		(PhaseTag == LuxPhaseTags::Phase_Action_Begin)				PhaseBegin(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_ProcessHit)			PhaseProcessHit(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Recovery)			PhaseRecovery(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Interrupt)			PhaseInterrupt(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_End)				PhaseEnd(SourceASC);
}

void UAuroraAction_FrozenSword::PhaseBegin(UActionSystemComponent& SourceASC)
{
	APawn* AvatarPawn = Cast<APawn>(SourceASC.GetAvatarActor());
	if (!AvatarPawn)
	{
		EndAction();
		return;
	}

	FLuxActionSpec* Spec = GetLuxActionSpec();
	if (!Spec)
	{
		EndAction();
		return;
	}

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	int32 CurrentComboCount = SourceASC.GetTagStackCount(ComboCountTag);
	if (GameState->GetServerWorldTimeSeconds() - Spec->LastExecutionTime >= ComboResetTime)
	{
		CurrentComboCount = 1;
	}
	
	if (CurrentComboCount <= 0 || CurrentComboCount > 4)
	{
		CurrentComboCount = 1;
	}

	// 공격 속도에 따라 몽타주 재생
	PlayMontageWithAttackSpeed(SourceASC, CurrentComboCount);

	int32 NextComboCount = (CurrentComboCount % 4) + 1;
	SourceASC.RemoveTag(ComboCountTag, 99);
	SourceASC.AddTag(ComboCountTag, NextComboCount);

	FGameplayTagContainer TagsToBlock;
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Action);
	AddTags(TagsToBlock, 1);

	// 서버에서만 HitPredicted 이벤트에 구독
	if (AvatarPawn->HasAuthority())
	{
		SUBSCRIBE_TO_TASK_EVENT(LuxGameplayTags::Task_Event_Attack_HitPredicted, Server_OnHitPredicted);
		UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s][%s] 서버에서 HitPredicted 이벤트 구독 완료"), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
	}

	// 클라이언트에서만 서버 액션 실행 상태를 확인하는 WaitForServer 태스크 생성
	if (AvatarPawn->IsLocallyControlled())
	{
		ULuxActionTask_WaitForServer* Task = ULuxActionTask_WaitForServer::WaitForServer(this, 0.02f);
		if (Task)
		{
			Task->OnServerActionActive.AddDynamic(this, &UAuroraAction_FrozenSword::Client_PredictHitCheck);
		}
	}
}

void UAuroraAction_FrozenSword::PlayMontageWithAttackSpeed(UActionSystemComponent& SourceASC, int32 ComboCount)
{
	// 공격 속도에 따라 재생 속도 조절합니다 (기본 공격 속도를 1.0으로 가정하고 계산)
	float AttackSpeed = 1.0f;
	if (const UCombatSet* CombatSet = SourceASC.GetAttributeSet<UCombatSet>())
	{
		AttackSpeed = CombatSet->GetAttackSpeed();
	}

	float AnimLength = MontageToPlay->GetSectionLength(ComboCount);
	switch (ComboCount)
	{
	case 1:
	case 2:
	case 3:
		AnimLength = 1.0f;
		break;
	case 4:
		AnimLength = 0.5f;
		break;
	default:
		AnimLength = 1.0f;
		break;
	}

	// 현재 공격 속도로 실제 공격에 걸리는 시간을 계산합니다. (예: 공격 속도 2.0 -> 0.5초당 1회 공격)
	const float AttackIntervalTime = 1.0f / AttackSpeed;

	// 애니메이션 길이를 목표 시간으로 나누어 필요한 배속을 계산합니다.
	// 예: 1초짜리 애니메이션을 0.5초 안에 끝내려면 PlayRate = 1.0 / 0.5 = 2.0 (2배속)
	// 예: 1초짜리 애니메이션을 2.0초에 걸쳐 보여주려면 PlayRate = 1.0 / 2.0 = 0.5 (0.5배속)
	float PlayRate = (AttackIntervalTime < AnimLength) ? (AnimLength / AttackIntervalTime) : 1.0f;

	const float MinPlayRate = 0.5f;
	const float MaxPlayRate = 2.0f;
	PlayRate = FMath::Clamp(PlayRate, MinPlayRate, MaxPlayRate);

	const FName SectionName = *FString::Printf(TEXT("Attack%d"), ComboCount);
	ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(this, MontageToPlay, PlayRate, SectionName);
}

void UAuroraAction_FrozenSword::Client_PredictHitCheck()
{
	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][%s] 서버 액션 실행 대기 완료, 타겟 검출 시작"), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] ActionSystemComponent가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	ACharacter* SourceAvatar = Cast<ACharacter>(ASC->GetAvatarActor());
	if (!SourceAvatar) 
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s]  SourceASC '%s' 의 아바타 엑터가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *ASC->GetName());
		return;
	}

	ILuxTargetingInterface* TargetingInterface = Cast<ILuxTargetingInterface>(SourceAvatar);
	if (!TargetingInterface)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s]  SourceASC '%s' 의 타겟팅 인터페이스가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *ASC->GetName());
		return;
	}

	UTargetingComponent* TargetingComponent = TargetingInterface->GetTargetingComponent();
	if (!TargetingComponent)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s]  SourceASC '%s' 의 타겟팅 컴포넌트가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *ASC->GetName());
		return;
	}

	AActor* TargetActor = TargetingComponent->GetCurrentTarget();
	if (!TargetActor)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s]  SourceASC '%s' 의 타겟 액터가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *ASC->GetName());
		return;
	}

	// 적대적인 관계일 때만 로직을 실행합니다.
	/*if (ULuxTeamStatics::GetTeamAttitude(SourceAvatar, TargetActor) != ETeamAttitude::Hostile)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s]  SourceASC '%s' 의 타겟 액터가 적대적인 관계가 아닙니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *SourceASC.GetName());
		return;
	}*/

	// 페이로드에 타겟 액터를 담습니다.
	FPayload_HitData HitPayload;
	HitPayload.SetData(TargetActor);

	FContextPayload ContextPayload;
	ContextPayload.SetData(LuxPayloadKeys::HitData, HitPayload);

	// [클라이언트 예측 및 서버 요청]
	PostTaskEvent(LuxGameplayTags::Task_Event_Attack_HitPredicted, ContextPayload);

	UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 클라이언트 예측 및 서버 요청 완료. 타겟 액터: %s"), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *TargetActor->GetName());

	// 로컬 클라이언트에서는 즉시 시각적 피드백을 표시합니다.
	if (SourceAvatar->IsLocallyControlled())
	{
		
	}
}

void UAuroraAction_FrozenSword::Server_OnHitPredicted(const FGameplayTag& EventTag, const FContextPayload& Payload)
{
	if (EventTag != LuxGameplayTags::Task_Event_Attack_HitPredicted)
		return;

	if(!GetAvatarActor() || !GetAvatarActor()->HasAuthority())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버에서 액션이 실행되지 않거나 권한이 없습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	// 구독 해제 - 매크로로 구독한 이벤트 수동 해제
	UNSUBSCRIBE_FROM_TASK_EVENT(LuxGameplayTags::Task_Event_Attack_HitPredicted, Server_OnHitPredicted);

	// 페이로드 저장만 하고, 노티파이 시점에 ProcessHit에서 처리
	const FPayload_HitData* HitData = Payload.GetData<FPayload_HitData>(LuxPayloadKeys::HitData);
	if (HitData)
	{
		ActionPayload->SetData(LuxPayloadKeys::HitData, *HitData);
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 타겟 액터: %s"), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *HitData->TargetActor.Get()->GetName());
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
	}
}

void UAuroraAction_FrozenSword::PhaseProcessHit(UActionSystemComponent& SourceASC)
{
	APawn* AvatarPawn = Cast<APawn>(SourceASC.GetAvatarActor());
	if (!AvatarPawn)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 아바타 엑터가 Pawn 타입이 아닙니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	// 이 페이즈는 서버에서만 의미가 있습니다.
	if (!AvatarPawn->HasAuthority())
	{
		return;
	}

	// 서버의 OnHitPredicted 함수가 ActionPayload에 저장해 둔 타격 정보를 가져옵니다.
	const FPayload_HitData* HitData = ActionPayload->GetData<FPayload_HitData>(FName("HitData"));
	if (!HitData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	ILuxTargetingInterface* TargetingInterface = Cast<ILuxTargetingInterface>(AvatarPawn);
	if (!TargetingInterface)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s]  SourceASC '%s' 의 타겟팅 인터페이스가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *SourceASC.GetName());
		return;
	}

	UTargetingComponent* TargetingComponent = TargetingInterface->GetTargetingComponent();
	if (!TargetingComponent)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s]  SourceASC '%s' 의 타겟팅 컴포넌트가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *SourceASC.GetName());
		return;
	}

	//AActor* TargetActor = TargetingComponent->GetCurrentTarget();
	AActor* TargetActor = HitData->TargetActor.Get();
	if (!TargetActor)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 타겟 액터가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	// 적대 관계인지 다시 한번 확인합니다.
	/*if (ULuxTeamStatics::GetTeamAttitude(AvatarPawn, TargetActor) != ETeamAttitude::Hostile)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 타겟 액터가 적대 관계가 아닙니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}*/

	IActionSystemInterface* TargetASI = Cast<IActionSystemInterface>(TargetActor);
	if (!TargetASI)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 타겟 액터가 IActionSystemInterface를 구현하지 않았습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	UActionSystemComponent* TargetASC = TargetASI->GetActionSystemComponent();
	if (!TargetASC || &SourceASC == TargetASC)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 타겟 ASC가 유효하지 않거나 소스 ASC와 같습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	FLuxActionSpec* ActionSpec = GetLuxActionSpec();
	if (!ActionSpec)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 액션 스펙이 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	// --- 실제 데미지 처리 로직 ---
	const int32 CurrentLevel = ActionSpec->Level;
	const FName RowName = FName(*FString::FromInt(CurrentLevel));
	const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT(""));
	if (!LevelData)
	{
		EndAction();
		return;
	}

	const FActionLevelDataBase* LevelDataBase = LevelData->ActionSpecificData.GetPtr<FActionLevelDataBase>();
	if (!LevelDataBase)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 레벨 데이터가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	ULuxCombatManager* CombatManager = GetWorld()->GetSubsystem<ULuxCombatManager>();
	if (!CombatManager)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 CombatManager가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	TSubclassOf<ULuxEffect> DamageEffect = CombatManager->GetDefaultDamageEffect();
	if (!DamageEffect)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 데미지 이펙트가 유효하지 않습니다."), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	FLuxEffectContextHandle EffectContext = SourceASC.MakeEffectContext();
	EffectContext.SetTargetASC(TargetASC);
	EffectContext.SetSourceAction(ActionSpec->Handle);

	FLuxEffectSpecHandle DamageSpec = SourceASC.MakeOutgoingSpec(DamageEffect, CurrentLevel, EffectContext);
	if (DamageSpec.IsValid())
	{
		DamageSpec.Get()->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Base, LevelDataBase->PhysicalDamage);
		DamageSpec.Get()->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Base, LevelDataBase->MagicalDamage);
		DamageSpec.Get()->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Scale, LevelDataBase->PhysicalScale);
		DamageSpec.Get()->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Scale, LevelDataBase->MagicalScale);
	}

	bool bResult = CombatManager->ApplyDamage(&SourceASC, TargetASC, DamageSpec);
	if (bResult)
	{
		FLuxCueContext CueContext;
		CueContext.Instigator = AvatarPawn;
		CueContext.Location = AvatarPawn->GetActorLocation();
		CueContext.Rotation = AvatarPawn->GetActorRotation();
		CueContext.EffectCauser = GetOwnerActor();
		CueContext.SourceASC = &SourceASC;
		CueContext.Level = CurrentLevel;
		CueContext.Magnitude = DamageSpec.Get()->GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magnitude); // 데미지 계산 후 피해량

		SourceASC.ExecuteGameplayCue(TargetActor, LuxCueTags::Cue_Aurora_FrozenSword_Primary, CueContext);
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s][%s] 서버 타격 정보에 데미지 처리 완료. 타겟 액터: %s 데미지: %f"), *GetLogPrefix(), ANSI_TO_TCHAR(__FUNCTION__), *TargetActor->GetName(), CueContext.Magnitude	);
	}
}

void UAuroraAction_FrozenSword::PhaseRecovery(UActionSystemComponent& SourceASC)
{
	RemoveTag(LuxGameplayTags::State_Block_Action, 1);
}

void UAuroraAction_FrozenSword::PhaseInterrupt(UActionSystemComponent& SourceASC)
{
	for (ULuxActionTask* ActiveTask : ActiveTasks)
	{
		if (ULuxActionTask_PlayMontageAndWait* MontageTask = Cast<ULuxActionTask_PlayMontageAndWait>(ActiveTask))
		{
			MontageTask->EndTask(false);
			break;
		}
	}
}

void UAuroraAction_FrozenSword::PhaseEnd(UActionSystemComponent& SourceASC)
{
	EndAction();
}