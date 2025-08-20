// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Actions/Aurora/AuroraAction_Cryoseism.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Tasks/LuxActionTask_PlayMontageAndWait.h"
#include "ActionSystem/Tasks/LuxActionTask_LandingControl.h"
#include "Actors/Action/Aurora/CryoseismExplosion.h" 

#include "Cues/LuxCueTags.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAuroraAction_Cryoseism::UAuroraAction_Cryoseism()
{
	InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerExecution;
	ActivationPolicy = ELuxActionActivationPolicy::OnInputTriggered;

	ActionIdentifierTag = LuxActionTags::Action_Aurora_Cryoseism;

	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Ultimate);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Damaging);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_CrowdControl);
	ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Debuff);

	static ConstructorHelpers::FObjectFinder<UDataTable> LevelDataFinder(TEXT("/Game/Characters/Aurora/DT_Aurora.DT_Aurora"));
	if (LevelDataFinder.Succeeded()) LevelDataTable = LevelDataFinder.Object;

	/** =================== 'Begin' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'StartBlast' 노티파이 발생 시 'Execute' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		Transition.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_Execute;

		// 조건: Notify 이름이 'StartBlast'인 경우
		FInstancedStruct Condition = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		Condition.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartBlast");
		Transition.Conditions.Add(Condition);

		// 예외: 시간 초과 시 'Interrupt' 페이즈로 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 3.0f; // 3초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Interrupt;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(TimeoutTransition);
	}

	/** =================== 'Execute' 페이즈 전환 규칙 =================== */
	{
		// 성공: 'StartLanding' 노티파이 발생 시 'Landing' 페이즈로 전환
		FPhaseTransition Transition;
		Transition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		Transition.EventTag = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
		Transition.NextPhaseTag = LuxPhaseTags::Phase_Action_Landing;

		// 조건: Notify 이름이 'StartLanding'인 경우
		FInstancedStruct Condition = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
		Condition.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartLanding");
		Transition.Conditions.Add(Condition);

		// 예외: 시간 초과 시 'Landing' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 2.0f; // 2초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Landing;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Execute).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Execute).Add(TimeoutTransition);
	}

	/** =================== 'Landing' 페이즈 전환 규칙 =================== */
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
		TimeoutTransition.Duration = 1.5f; // 1.5초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Recovery;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Landing).Add(Transition);
		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Landing).Add(TimeoutTransition);
	}

	/** =================== 'Recovery' 페이즈 전환 규칙 =================== */
	{
		// 성공: 애니메이션 종료 시 'End' 페이즈로 전환
		FPhaseTransition OnMontageEndedTransition;
		OnMontageEndedTransition.TransitionType = EPhaseTransitionType::OnTaskEvent;
		OnMontageEndedTransition.EventTag = LuxGameplayTags::Task_Event_Montage_Ended;
		OnMontageEndedTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		// 캔슬: 이동 입력이 들어오면 'Interrupt' 페이즈로 전환
		FPhaseTransition MovementInterrupt;
		MovementInterrupt.TransitionType = EPhaseTransitionType::OnGameplayEvent;
		MovementInterrupt.EventTag = LuxGameplayTags::Event_Movement_Started;
		MovementInterrupt.NextPhaseTag = LuxPhaseTags::Phase_Action_Interrupt;

		// 예외: 시간 초과 시 'End' 페이즈로 강제 전환
		FPhaseTransition TimeoutTransition;
		TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
		TimeoutTransition.Duration = 2.0f; // 2초 후 시간 초과
		TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

		PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(OnMontageEndedTransition);
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

void UAuroraAction_Cryoseism::OnActionEnd(bool bIsCancelled)
{
	FGameplayTagContainer TagsToUnblock;
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Action);
	RemoveTags(TagsToUnblock, 1);

	// 액션 종료 시 중력을 복원합니다.
	EnableGravity();

	Super::OnActionEnd(bIsCancelled);
}


void UAuroraAction_Cryoseism::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
	Super::OnPhaseEnter(PhaseTag, SourceASC);

	if		(PhaseTag == LuxPhaseTags::Phase_Action_Begin)			PhaseLeap(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Execute)		PhaseImpact(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Landing)		PhaseLanding(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Recovery)		PhaseRecovery(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_Interrupt)		PhaseInterrupt(SourceASC);
	else if (PhaseTag == LuxPhaseTags::Phase_Action_End)			PhaseEnd(SourceASC);
}

void UAuroraAction_Cryoseism::PhaseLeap(UActionSystemComponent& SourceASC)
{
	AActor* AvatarActor = SourceASC.GetAvatarActor();
	if (!AvatarActor) return;

	if (AvatarActor->HasAuthority())
	{
		// 현재 액션의 레벨 데이터를 가져옵니다.
		const int32 CurrentLevel = GetActionLevel();
		const FName RowName = FName(*FString::FromInt(CurrentLevel));
		const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT(""));
		if (!LevelData)
		{
			EndAction();
			return;
		}

		// Cryoseism 전용 레벨 데이터를 가져옵니다.
		const FAuroraActionLevelData_Cryoseism* CryoseismLevelData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_Cryoseism>();
		if (!CryoseismLevelData)
		{
			EndAction();
			return;
		}

		ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
		if (Character && !Character->GetCharacterMovement()->IsFalling())
		{
			Character->LaunchCharacter(FVector(0, 0, CryoseismLevelData->LeapVelocity), false, true);
		}
	}

	ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(this, MontageToPlay);

	FGameplayTagContainer TagsToBlock;
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToBlock.AddTag(LuxGameplayTags::State_Block_Action);

	AddTags(TagsToBlock, 1);
}

void UAuroraAction_Cryoseism::PhaseImpact(UActionSystemComponent& SourceASC)
{
	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor) return;

	if (!AvatarActor->HasAuthority()) 
		return;

	// 폭발 액터를 소환하고 초기화합니다.
	if (!ExplosionClass)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 ExplosionClass가 지정되지 않았습니다. 로드를 시도합니다."), *GetName());
		FName Path = TEXT("/Game/Characters/Aurora/AuroraHoarfrost.AuroraHoarfrost_C");
		ExplosionClass = LoadClass<ACryoseismExplosion>(nullptr, *Path.ToString());
	}
	
	if (!ExplosionClass)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 ExplosionClass를 로드할 수 없습니다. 액션을 종료합니다."), *GetName());
		EndAction();
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwnerActor();
	SpawnParams.Instigator = GetAvatarActor()->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ACryoseismExplosion* CryoseismExplosion = GetWorld()->SpawnActor<ACryoseismExplosion>(
		ExplosionClass,
		GetAvatarActor()->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (CryoseismExplosion)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: 폭발 액터를 생성했습니다."));
		CryoseismExplosion->Initialize(this, true);
		DisableGravity();
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: 폭발 액터를 생성할 수 없습니다."));
		return;
	}
}

void UAuroraAction_Cryoseism::PhaseLanding(UActionSystemComponent& SourceASC)
{
	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor) return;

	// 현재 액션의 레벨 데이터를 가져옵니다.
	const int32 CurrentLevel = GetActionLevel();
	const FName RowName = FName(*FString::FromInt(CurrentLevel));
	const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT(""));
	if (!LevelData)
	{
		EndAction();
		return;
	}

	// Cryoseism 전용 레벨 데이터를 가져옵니다.
	const FAuroraActionLevelData_Cryoseism* CryoseismLevelData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_Cryoseism>();
	if (!CryoseismLevelData)
	{
		EndAction();
		return;
	}

	// 서버에서만 착지 로직을 실행합니다.
	if (AvatarActor->HasAuthority())
	{
		// 중력을 재활성화합니다.
		EnableGravity();
		
		ACharacter* Character = Cast<ACharacter>(AvatarActor);
		if (Character && Character->GetCharacterMovement()->IsFalling())
		{
			// 다음 노티파이까지의 시간을 계산합니다.
			float TimeToNextNotify = CalculateTimeToNextNotify(FName("StartRecovery"));

			// 땅까지의 거리를 계산합니다.
			float DistanceToGround = CalculateDistanceToGround(Character);

			// 중력 가속도 고려 없이 일정한 속도로 착지하기 위한 속도를 계산합니다.
			float RequiredVelocity = CalculateConstantVelocity(DistanceToGround, TimeToNextNotify);

			// 최대 속도 제한을 적용합니다.
			float FinalLandingVelocity = FMath::Max(RequiredVelocity, CryoseismLevelData->LandingVelocity);

			// 착지 제어 태스크를 시작합니다 (자연스러운 착지 사용).
			ULuxActionTask_LandingControl::LandingControl(
				this,
				FinalLandingVelocity,
				TimeToNextNotify,
				5.0,
				false
			);

			UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] PhaseLanding: 착지 제어 시작 - TimeToNextNotify: %.2f, DistanceToGround: %.2f, FinalLandingVelocity: %.2f"),
				*GetName(), TimeToNextNotify, DistanceToGround, FinalLandingVelocity);
		}
	}
}


void UAuroraAction_Cryoseism::PhaseRecovery(UActionSystemComponent& SourceASC)
{
	FGameplayTagContainer TagsToUnblock;
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Action);

	RemoveTags(TagsToUnblock, 1);
}

void UAuroraAction_Cryoseism::PhaseInterrupt(UActionSystemComponent& SourceASC)
{
	// 모든 활성 태스크를 중단시킵니다.
	TArray<ULuxActionTask*> TasksToEnd = ActiveTasks;
	for (ULuxActionTask* ActiveTask : TasksToEnd)
	{
		if (ActiveTask)
		{
			ActiveTask->EndTask(true);
		}
	}
}

void UAuroraAction_Cryoseism::PhaseEnd(UActionSystemComponent& SourceASC)
{
	EndAction();
}



float UAuroraAction_Cryoseism::CalculateTimeToNextNotify(FName NotifyName)
{
	if (!MontageToPlay)
	{
		return 1.0f; // 기본값
	}

	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC)
	{
		return 1.0f;
	}

	ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor());
	if (!Character || !Character->GetMesh())
	{
		return 1.0f;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return 1.0f;
	}

	// 현재 몽타주 재생 시간을 가져옵니다.
	float CurrentTime = AnimInstance->Montage_GetPosition(MontageToPlay);
	
	// 몽타주에서 목표 노티파이의 시간을 찾습니다.
	float NotifyTime = 0.0f;
	bool bFoundNotify = false;
	
	// 몽타주의 모든 노티파이를 검사합니다.
	for (const FAnimNotifyEvent& NotifyEvent : MontageToPlay->Notifies)
	{
		if (NotifyEvent.Notify->GetNotifyName() == NotifyName)
		{
			UE_LOG(LogLuxActionSystem, Error, TEXT("NotifyEvent: %s"), *NotifyEvent.Notify->GetNotifyName());
			NotifyTime = NotifyEvent.GetTime();
			bFoundNotify = true;
			break;
		}
	}
	
	if (!bFoundNotify)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s] CalculateTimeToNextNotify: 노티파이 '%s'를 찾을 수 없습니다."), 
			*GetName(), *NotifyName.ToString());
		return 1.0f;
	}
	
	// 다음 노티파이까지의 시간을 계산합니다.
	float TimeToNotify = NotifyTime - CurrentTime;
	
	// 음수인 경우 이미 지나간 노티파이이므로 기본값을 사용합니다.
	if (TimeToNotify <= 0.0f)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s] CalculateTimeToNextNotify: 노티파이 '%s'가 이미 지나갔습니다. 기본값을 사용합니다."), 
			*GetName(), *NotifyName.ToString());
		return 1.0f;
	}
	
	return TimeToNotify;
}

/* ==== 거리 및 속도 계산 함수 ==== */

float UAuroraAction_Cryoseism::CalculateDistanceToGround(ACharacter* Character)
{
	if (!Character)
	{
		return 1000.0f; // 기본값
	}

	FVector StartLocation = Character->GetActorLocation();
	FVector EndLocation = StartLocation + FVector(0, 0, -10000.0f); // 아래로 충분히 긴 거리

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	bool bHit = Character->GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_WorldStatic,
		QueryParams
	);

	if (bHit)
	{
		float DistanceToGround = FVector::Dist(StartLocation, HitResult.Location);
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] CalculateDistanceToGround: 땅까지 거리 %.2f"), 
			*GetName(), DistanceToGround);
		return DistanceToGround;
	}

	UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s] CalculateDistanceToGround: 땅을 찾을 수 없어 기본값 사용"), *GetName());
	return 1000.0f; // 기본값
}

float UAuroraAction_Cryoseism::CalculateConstantVelocity(float Distance, float Time)
{
	if (Time <= 0.0f)
	{
		return -800.0f; // 기본값
	}

	// 중력 가속도 고려 없이 일정한 속도로 계산: v = distance / time
	float RequiredVelocity = Distance / Time;

	// 음수 속도로 변환 (아래로 떨어지는 방향)
	RequiredVelocity = -FMath::Abs(RequiredVelocity);

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] CalculateConstantVelocity: Distance=%.2f, Time=%.2f, Velocity=%.2f"), 
		*GetName(), Distance, Time, RequiredVelocity);

	return RequiredVelocity;
}

float UAuroraAction_Cryoseism::CalculateRequiredVelocity(float StartHeight, float LandingTime)
{
	if (LandingTime <= 0.0f)
	{
		return -800.0f; // 기본값
	}

	// 중력 가속도를 고려하여 필요한 속도를 계산합니다.
	// h = v0 * t + 0.5 * g * t^2
	// v0 = (h - 0.5 * g * t^2) / t
	// 여기서 h는 높이, g는 중력 가속도, t는 시간입니다.

	const float Gravity = 980.0f; // Unreal Engine의 기본 중력 가속도
	float RequiredVelocity = (StartHeight - 0.5f * Gravity * LandingTime * LandingTime) / LandingTime;

	// 음수 속도로 변환 (아래로 떨어지는 방향)
	RequiredVelocity = -FMath::Abs(RequiredVelocity);

	return RequiredVelocity;
}

/* ==== 중력 제어 함수들 ==== */

void UAuroraAction_Cryoseism::DisableGravity()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (!Character || !Character->GetCharacterMovement())
	{
		return;
	}

	// 이미 비활성화되었다면 건너뛰기
	if (bGravityDisabled)
	{
		return;
	}

	// 원래 중력 스케일을 저장하고 중력을 비활성화
	OriginalGravityScale = Character->GetCharacterMovement()->GravityScale;
	Character->GetCharacterMovement()->GravityScale = 0.0f;
	bGravityDisabled = true;

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] DisableGravity: 중력을 비활성화했습니다. (원래 스케일: %.2f)"), 
		*GetName(), OriginalGravityScale);
}

void UAuroraAction_Cryoseism::EnableGravity()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (!Character || !Character->GetCharacterMovement())
	{
		return;
	}

	// 중력이 비활성화되지 않았다면 건너뛰기
	if (!bGravityDisabled)
	{
		return;
	}

	// 원래 중력 스케일을 복원
	Character->GetCharacterMovement()->GravityScale = OriginalGravityScale;
	bGravityDisabled = false;

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] EnableGravity: 중력을 재활성화했습니다. (스케일: %.2f)"), 
		*GetName(), OriginalGravityScale);
}