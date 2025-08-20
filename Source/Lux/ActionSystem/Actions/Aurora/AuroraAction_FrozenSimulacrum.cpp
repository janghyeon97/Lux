// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Actions/Aurora/AuroraAction_FrozenSimulacrum.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Tasks/LuxActionTask_PlayMontageAndWait.h"
#include "ActionSystem/Tasks/LuxActionTask_LeapToLocation.h" 
#include "Actors/Action/Aurora/FrozenSimulacrum.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

UAuroraAction_FrozenSimulacrum::UAuroraAction_FrozenSimulacrum()
{
    InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerExecution;
    ActivationPolicy = ELuxActionActivationPolicy::OnInputTriggered;

    ActionIdentifierTag = LuxActionTags::Action_Aurora_FrozenSimulacrum;
    ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability);
    ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Normal);
    ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability_Movement);

    /** =================== 'Begin' 페이즈 전환 규칙 =================== */
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
        TimeoutTransition.Duration = 2.5f; // 2.5초 후 시간 초과
        TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Recovery;

        PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(Transition);
        PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(TimeoutTransition);
    }

    /** =================== 'Recovery' 페이즈 전환 규칙 =================== */
    {
        // 성공: 애니메이션 종료 시 'End' 페이즈로 전환
        FPhaseTransition OnMontageEndedTransition;
        OnMontageEndedTransition.TransitionType = EPhaseTransitionType::OnTaskEvent;
        OnMontageEndedTransition.EventTag = LuxGameplayTags::Task_Event_Montage_Ended;
        OnMontageEndedTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

        // 캔슬: 이동 입력이 들어오면 'Interrupt' 페이즈로 전환
        FPhaseTransition MovementInterruptTransition;
        MovementInterruptTransition.TransitionType = EPhaseTransitionType::OnGameplayEvent;
        MovementInterruptTransition.EventTag = LuxGameplayTags::Event_Movement_Started;
        MovementInterruptTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_Interrupt;

        // 예외: 시간 초과 시 'End' 페이즈로 전환
        FPhaseTransition TimeoutTransition;
        TimeoutTransition.TransitionType = EPhaseTransitionType::OnDurationEnd;
        TimeoutTransition.Duration = 2.0f;
        TimeoutTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;

        PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(TimeoutTransition);
        PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(OnMontageEndedTransition);
        PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Recovery).Add(MovementInterruptTransition);
    }

    /** =================== 'Interrupt' 페이즈 전환 규칙 =================== */
    {
		// 성공: 'End' 페이즈로 즉시 전환
        FPhaseTransition ImmediateEndTransition;
        ImmediateEndTransition.TransitionType = EPhaseTransitionType::Immediate;
        ImmediateEndTransition.NextPhaseTag = LuxPhaseTags::Phase_Action_End;
        PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Interrupt).Add(ImmediateEndTransition);
    }
}

void UAuroraAction_FrozenSimulacrum::OnActionEnd(bool bIsCancelled)
{
    if (UActionSystemComponent* ASC = GetActionSystemComponent())
    {
        FGameplayTagContainer TagsToUnblock;
        TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Movement);
        TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Rotation);
        TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Action);

        RemoveTags(TagsToUnblock, 1);

        // 매크로로 구독한 이벤트 수동 해제
        UNSUBSCRIBE_FROM_GAMEPLAY_EVENT(LuxGameplayTags::Event_Movement_Started, HandleGameplayEventForInterruption);
    }

    Super::OnActionEnd(bIsCancelled);
}

void UAuroraAction_FrozenSimulacrum::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
    Super::OnPhaseEnter(PhaseTag, SourceASC);

    if      (PhaseTag == LuxPhaseTags::Phase_Action_Begin)          PhaseLeap(SourceASC);
    else if (PhaseTag == LuxPhaseTags::Phase_Action_Recovery)       PhaseRecovery(SourceASC);
    else if (PhaseTag == LuxPhaseTags::Phase_Action_Interrupt)      PhaseInterrupt(SourceASC);
    else if (PhaseTag == LuxPhaseTags::Phase_Action_End)            PhaseEnd(SourceASC);
}

void UAuroraAction_FrozenSimulacrum::PhaseLeap(UActionSystemComponent& SourceASC)
{
    ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActor());
    if (!AvatarCharacter)
    {
		UE_LOG(LogLuxActionSystem, Error, TEXT("AvatarActor가 유효하지 않습니다. 액션을 종료합니다."));
		EndAction();
        return;
    }

    const int32 CurrentLevel = GetActionLevel();
    const FName RowName = FName(*FString::FromInt(CurrentLevel));
    const FLuxActionLevelData* LevelData = LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("GetFrozenSimulacrumLevelData"));
    if (!LevelData)
    {
        UE_LOG(LogLuxActionSystem, Error, TEXT("'%s' GetFrozenSimulacrumLevelData: LevelDataTable에서 Level '%d'에 해당하는 데이터를 찾을 수 없습니다."), *GetName(), CurrentLevel);
        EndAction();
        return;
    }

    const FAuroraActionLevelData_FrozenSimulacrum* FrozenSimulacrumData = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_FrozenSimulacrum>();
    if (!FrozenSimulacrumData)
    {
        UE_LOG(LogLuxActionSystem, Error, TEXT("'%s' GetFrozenSimulacrumLevelData: Level '%d'의 데이터가 FAuroraActionLevelData_FrozenSimulacrum 타입이 아닙니다."), *GetName(), CurrentLevel);
        EndAction();
        return;
    }

    /*if (!SimulacrumClass)
    {
        UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 SimulacrumClass가 지정되지 않았습니다. 로드를 시도합니다."), *GetName());
        FName Path = TEXT("/Game/Characters/Aurora/AuroraHoarfrost.AuroraHoarfrost_C");
        SimulacrumClass = LoadClass<AFrozenSimulacrum>(nullptr, *Path.ToString());
    }*/

    if (!SimulacrumClass)
    {
        UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 SimulacrumClass를 로드할 수 없습니다. 액션을 종료합니다."), *GetName());
        EndAction();
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogLuxActionSystem, Error, TEXT("'%s'에 World를 찾을 수 없습니다. 액션을 종료합니다."), *GetName());
        EndAction();
        return;
    }

    // 서버에서만 실제 로직을 실행합니다.
    if (AvatarCharacter->HasAuthority())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwnerActor();
        SpawnParams.Instigator = AvatarCharacter->GetInstigator();
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        FrozenSimulacrum = GetWorld()->SpawnActor<AFrozenSimulacrum>(
            SimulacrumClass,
            AvatarCharacter->GetActorTransform(),
            SpawnParams
        );

        if (FrozenSimulacrum)
        {
            FrozenSimulacrum->Initialize(this, true);
        }

        FVector LeapDirection = AvatarCharacter->GetCharacterMovement()->Velocity.GetSafeNormal();

        // 만약 제자리에 서 있었다면 컨트롤러가 바라보는 방향을 사용합니다.
        if (LeapDirection.IsNearlyZero())
        {
            LeapDirection = AvatarCharacter->GetControlRotation().Vector();
        }

        const FVector Destination = CalculateLeapDestination(AvatarCharacter, LeapDirection, FrozenSimulacrumData->LeapDistance, FrozenSimulacrumData->LeapHeightThreshold);

        if (!Destination.IsNearlyZero())
        {
            ULuxActionTask_LeapToLocation::LeapToLocation(this, Destination, FrozenSimulacrumData->LeapDuration, FrozenSimulacrumData->LeapHeight);
        }

        FGameplayTagContainer TagsToBlock;
        TagsToBlock.AddTag(LuxGameplayTags::State_Block_Movement);
        TagsToBlock.AddTag(LuxGameplayTags::State_Block_Rotation);
        TagsToBlock.AddTag(LuxGameplayTags::State_Block_Action);

        AddTags(TagsToBlock, 1);
    }

    // 계산된 방향 인덱스를 사용하여 몽타주의 시작 섹션을 결정합니다.
    FName MontageSectionName = FName(*FString::Printf(TEXT("Leap_Direction_%d"), CalculateDirectionIndex(AvatarCharacter)));
    ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(this, MontageToPlay, 1.0f, MontageSectionName);
    UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] '%s' PhaseLeap: MontageSectionName: %s"), *ClientServerString, *GetName(), *MontageSectionName.ToString());

}

FVector UAuroraAction_FrozenSimulacrum::CalculateLeapDestination(ACharacter* Character, const FVector& Direction, float Range, float HeightThreshold)
{
    UWorld* World = GetWorld();
    if (!Character || !World) return FVector::ZeroVector;

    const FVector StartLocation = Character->GetActorLocation();
    const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

    FCollisionQueryParams CollisionParams(NAME_None, false, Character);
    CollisionParams.AddIgnoredActor(FrozenSimulacrum);

    FVector FarthestValidLocation = StartLocation; // 현재까지 발견된 가장 먼 유효 착지 지점
    const int32 NumSteps = 10;
    const float StepSize = Range / NumSteps;

    // 가까운 지점부터 최대 거리까지 모든 지점을 순차적으로 탐색합니다.
    for (int i = 1; i <= NumSteps; ++i)
    {
        const float CurrentDistance = i * StepSize;
        const FVector ProbePoint = StartLocation + (Direction * CurrentDistance);

        FHitResult GroundHit;
        bool bGroundHit = World->LineTraceSingleByChannel(GroundHit, ProbePoint + FVector(0, 0, 10000.f), ProbePoint - FVector(0, 0, 10000.f), ECC_GameTraceChannel11, CollisionParams);

        // 유효성 검사: (A)땅이 있고, (B)낭떠러지가 아닌 지 확인합니다.
        if (bGroundHit && FMath::Abs(StartLocation.Z - GroundHit.Location.Z) < HeightThreshold)
        {
            FarthestValidLocation = GroundHit.ImpactPoint + FVector(0, 0, CapsuleHalfHeight);
            //DrawDebugSphere(World, ProbePoint, 20.f, 12, FColor::Green, false, 5.0f, 0, 1.0f);
        }
        else
        {
            //DrawDebugSphere(World, ProbePoint, 20.f, 12, FColor::Red, false, 5.0f, 0, 1.0f);
        }
    }

    //DrawDebugSphere(World, FarthestValidLocation, 30.f, 12, FColor::Purple, false, 5.0f, 0, 1.0f);
    return FarthestValidLocation;
}


int32 UAuroraAction_FrozenSimulacrum::CalculateDirectionIndex(ACharacter* TargetCharacter) const
{
    // 유효하지 않은 경우 기본값(정면)을 반환합니다.
    if (!TargetCharacter || !TargetCharacter->GetCharacterMovement())
    {
        return 1;
    }

    // 캐릭터의 현재 속도 방향과 조준 방향의 차이를 Yaw 값으로 계산합니다.
    float Yaw = UKismetMathLibrary::NormalizedDeltaRotator(
        UKismetMathLibrary::MakeRotFromX(TargetCharacter->GetCharacterMovement()->Velocity),
        TargetCharacter->GetBaseAimRotation()
    ).Yaw;

    // Yaw 값을 0-360 범위로 정규화하고 45도로 나누어 8방향 인덱스를 계산합니다.
    float NormalizedYaw = FMath::Fmod(Yaw + 360.0f, 360.0f);
    int32 DirectionIndex = FMath::RoundToInt(NormalizedYaw / 45.0f) % 8;

    // 인덱스는 1부터 시작하도록 1을 더해 반환합니다. (0-7 -> 1-8)
    return DirectionIndex + 1;
}


void UAuroraAction_FrozenSimulacrum::PhaseRecovery(UActionSystemComponent& SourceASC)
{
    SUBSCRIBE_TO_GAMEPLAY_EVENT(LuxGameplayTags::Event_Movement_Started, HandleGameplayEventForInterruption);

	FGameplayTagContainer TagsToUnblock;
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Movement);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Rotation);
	TagsToUnblock.AddTag(LuxGameplayTags::State_Block_Action);

	RemoveTags(TagsToUnblock, 1);
}

void UAuroraAction_FrozenSimulacrum::PhaseInterrupt(UActionSystemComponent& SourceASC)
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

void UAuroraAction_FrozenSimulacrum::PhaseEnd(UActionSystemComponent& SourceASC)
{
    EndAction();
}
