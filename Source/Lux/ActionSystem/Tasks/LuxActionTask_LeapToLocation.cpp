// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Tasks/LuxActionTask_LeapToLocation.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "LuxLogChannels.h"

ULuxActionTask_LeapToLocation* ULuxActionTask_LeapToLocation::LeapToLocation(ULuxAction* InOwningAction, FVector Destination, float Duration, float ArcHeight)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[LeapToLocation] LeapToLocationTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"LeapToLocationTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_LeapToLocation* NewTask = NewObject<ULuxActionTask_LeapToLocation>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("LeapToLocationTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("LeapToLocation");
	NewTask->OwningAction = InOwningAction;
	NewTask->TargetLocation = Destination;
	NewTask->LeapDuration = Duration;
	NewTask->LeapArcHeight = ArcHeight;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_LeapToLocation::OnActivated()
{
    Super::OnActivated();

    ACharacter* Character = Cast<ACharacter>(OwningAction->GetAvatarActor());
    if (!Character || !Character->GetCharacterMovement())
    {
        EndTask(false);
        return;
    }

    StartLocation = Character->GetActorLocation();
    PreviousLocation = StartLocation;
    OriginalMovementMode = Character->GetCharacterMovement()->MovementMode;
    Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    ElapsedTime = 0.0f;

    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ULuxActionTask_LeapToLocation::TickTask, 0.016f, true);
}

void ULuxActionTask_LeapToLocation::TickTask()
{
    const float TickInterval = 0.016f;
    ElapsedTime += TickInterval;
    float Alpha = FMath::Clamp(ElapsedTime / LeapDuration, 0.0f, 1.0f);

    ACharacter* Character = Cast<ACharacter>(OwningAction->GetAvatarActor());
    if (!Character)
    {
        EndTask(false);
        return;
    }

    // EaseInOut 커브를 사용하여 가속/감속 효과를 만듭니다.
    const float EaseAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

    // 포물선 Z 높이 계산 
    const float ParabolicZ = FMath::Sin(Alpha * PI) * LeapArcHeight;

    // 보간된 Alpha를 사용하여 이번 틱의 목표 위치를 계산합니다.
    FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, EaseAlpha);
    NewLocation.Z = FMath::Lerp(StartLocation.Z, TargetLocation.Z, Alpha) + ParabolicZ;

    const FVector NewVelocity = (NewLocation - PreviousLocation) / TickInterval;
    Character->GetCharacterMovement()->Velocity = NewVelocity;
    PreviousLocation = NewLocation;

    if (Alpha >= 1.0f)
    {
        Character->SetActorLocation(TargetLocation);
        EndTask(true);
    }
}

void ULuxActionTask_LeapToLocation::OnEnded(bool bSuccess)
{
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

    ACharacter* Character = Cast<ACharacter>(OwningAction->GetAvatarActor());
    if (Character && Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->SetMovementMode(OriginalMovementMode);
        Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    }

    Super::OnEnded(bSuccess);
}
