// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Tasks/LuxActionTask_FollowSpline.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "Character/LuxCharacter.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"
#include "TimerManager.h"

ULuxActionTask_FollowSpline* ULuxActionTask_FollowSpline::FollowSpline(ULuxAction* InOwningAction, USplineComponent* SplineToFollow, float Duration)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[FollowSpline] FollowSplineTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"FollowSplineTask failed: Non-Instanced Actions cannot create Tasks.");

	if (!SplineToFollow)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("FollowSplineTask failed for Action [%s] on SplineToFollow is null."), *InOwningAction->GetName());
		return nullptr;
	}

	ULuxActionTask_FollowSpline* NewTask = NewObject<ULuxActionTask_FollowSpline>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("FollowSplineTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("FollowSpline");
	NewTask->OwningAction = InOwningAction;
	NewTask->SplineToFollowPtr = SplineToFollow;
	NewTask->FollowDuration = Duration;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_FollowSpline::OnActivated()
{
    Super::OnActivated();

    ACharacter* Character = Cast<ACharacter>(OwningAction->GetAvatarActor());
    if (!Character || !SplineToFollowPtr.IsValid())
    {
        EndTask(false);
        return;
    }

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
    if (!MovementComp)
    {
        EndTask(false);
        return;
	}

    PreviousVelocity = MovementComp->Velocity;
    OriginalMovementMode = MovementComp->GetGroundMovementMode();
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->Velocity = FVector::ZeroVector;

    ElapsedTime = 0.0f;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ULuxActionTask_FollowSpline::TickTask, 0.016f, true); // 약 60fps로 실행
}

void ULuxActionTask_FollowSpline::TickTask()
{
    ElapsedTime += GetWorld()->GetTimerManager().GetTimerElapsed(TimerHandle);

    ACharacter* Character = Cast<ACharacter>(OwningAction->GetAvatarActor());
    UCharacterMovementComponent* MovementComp = Character ? Character->GetCharacterMovement() : nullptr;
    USplineComponent* Spline = SplineToFollowPtr.Get();

    if (!MovementComp || !Spline)
    {
        EndTask(false);
        return;
    }

    const float Alpha = FMath::Clamp(ElapsedTime / FollowDuration, 0.0f, 1.0f);
    const float Distance = Spline->GetSplineLength() * Alpha;

    // 이번 틱에서 도달해야 할 목표 위치를 계산합니다.
    FVector TargetLocationOnGround = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

    // 캐릭터 캡슐의 절반 높이만큼 Z축 오프셋을 추가합니다.
    if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
    {
        TargetLocationOnGround.Z += Capsule->GetScaledCapsuleHalfHeight() + 10.f;
    }

    // 현재 위치에서 목표 위치로 이동하기 위한 방향과 속력을 계산합니다.
    const FVector CurrentLocation = Character->GetActorLocation();
    const FVector Direction = (TargetLocationOnGround - CurrentLocation);
    const float TickInterval = GetWorld()->GetTimerManager().GetTimerRate(TimerHandle);

    // Velocity = Displacement / Time
    const FVector NewVelocity = Direction / TickInterval;
    
    // 속도 변화량을 기반으로 가속도를 계산합니다.
    const FVector NewAcceleration = (NewVelocity - PreviousVelocity) / TickInterval;

    MovementComp->Velocity = NewVelocity;

    if (Alpha >= 1.0f)
    {
        EndTask(true);
    }
}

void ULuxActionTask_FollowSpline::OnEnded(bool bSuccess)
{
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

    ACharacter* Character = Cast<ACharacter>(OwningAction->GetAvatarActor());
    if (Character)
    {
        Character->GetCharacterMovement()->SetMovementMode(OriginalMovementMode);
        Character->GetCharacterMovement()->Velocity = FVector(0, 0, -10.f);
    }

    if (OwningAction.IsValid())
    {
        FContextPayload ContextPayload;
        if (bSuccess)
        {
            OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_FollowPath_Completed, ContextPayload);
        }
        else
        {
            OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_FollowPath_Failed, ContextPayload);
        }
    }

    Super::OnEnded(bSuccess);
}