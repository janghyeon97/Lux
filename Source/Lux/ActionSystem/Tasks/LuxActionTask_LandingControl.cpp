// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSystem/Tasks/LuxActionTask_LandingControl.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "LuxLogChannels.h"

ULuxActionTask_LandingControl* ULuxActionTask_LandingControl::LandingControl(
	ULuxAction* InOwningAction,
	float InLandingVelocity,
	float InLandingDuration,
	float InInterpSpeed,
	bool InUseNaturalLanding)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] LandingControl failed: Owning Action is null."), *GetNameSafe(InOwningAction));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced &&
		"LandingControl failed: Non-Instanced Actions cannot create Tasks.");



	ULuxActionTask_LandingControl* NewTask = NewObject<ULuxActionTask_LandingControl>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("LandingControl failed for Action [%s]: NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("LandingControl");
	NewTask->OwningAction = InOwningAction;
	NewTask->LandingVelocity = InLandingVelocity;
	NewTask->LandingDuration = InLandingDuration;
	NewTask->InterpSpeed = InInterpSpeed;
	NewTask->bUseNaturalLanding = InUseNaturalLanding;
	NewTask->ElapsedTime = 0.0f;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_LandingControl::OnActivated()
{
	Super::OnActivated();

	UActionSystemComponent* ASC = OwningAction->GetActionSystemComponent();
	ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor());
	if (!ASC || !Character)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] OnActivated: ASC 또는 Character가 유효하지 않습니다."), *GetNameSafe(this));
		EndTask(false);
		return;
	}

	// 현재 높이를 저장합니다.
	StartHeight = Character->GetActorLocation().Z;
	CurrentHeight = StartHeight;

	// 착지 제어를 시작합니다.
	UpdateLandingControl();

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] OnActivated: 착지 제어 시작 - LandingVelocity: %.2f, LandingDuration: %.2f"), 
		*GetNameSafe(this), LandingVelocity, LandingDuration);
}

void ULuxActionTask_LandingControl::OnEnded(bool bSuccess)
{
	if (UpdateTimerHandle.IsValid())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().ClearTimer(UpdateTimerHandle);
		}
	}

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] OnEnded: 착지 제어 종료 - Success: %s"), *GetNameSafe(this), bSuccess ? TEXT("True") : TEXT("False"));

	Super::OnEnded(bSuccess);
}

void ULuxActionTask_LandingControl::OnBeforeReHome()
{
	if (UpdateTimerHandle.IsValid())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().ClearTimer(UpdateTimerHandle);
		}
	}

	Super::OnBeforeReHome();
}


void ULuxActionTask_LandingControl::UpdateLandingControl()
{
	UActionSystemComponent* ASC = OwningAction->GetActionSystemComponent();
	ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor());
	if (!ASC || !Character)
	{
		EndTask(false);
		return;
	}

	const float DeltaTime = 0.016f; // 약 60fps
	ElapsedTime += DeltaTime;
	CurrentHeight = Character->GetActorLocation().Z;

	// 착지했거나 지속 시간이 끝났으면 태스크 종료
	if (!Character->GetCharacterMovement()->IsFalling() || ElapsedTime >= LandingDuration)
	{
		UE_LOG(LogLuxActionSystem, Log, TEXT("[%s] UpdateLandingControl: 착지 제어 종료 - 착지: %s, 경과시간: %.2f/%.2f, 자연착지: %s"), 
			*GetNameSafe(this), Character->GetCharacterMovement()->IsFalling() ? TEXT("False") : TEXT("True"), ElapsedTime, LandingDuration, bUseNaturalLanding ? TEXT("True") : TEXT("False"));
		EndTask(true);
		return;
	}

	// 속도 계산 및 적용
	FVector CurrentVelocity = Character->GetCharacterMovement()->Velocity;
	CurrentVelocity.Z = bUseNaturalLanding ? CalculateNaturalLandingVelocity(CurrentVelocity.Z) : CalculateBasicLandingVelocity();
	Character->GetCharacterMovement()->Velocity = CurrentVelocity;

	// 다음 프레임에서 다시 업데이트
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &ULuxActionTask_LandingControl::UpdateLandingControl, DeltaTime, false);
	}
}


float ULuxActionTask_LandingControl::CalculateNaturalLandingVelocity(float CurrentVelocityZ)
{
	const float DeltaTime = 0.016f;
	
	// 보간으로 인한 속도 손실을 보상하기 위한 속도 보정 계수
	const float SpeedCompensationFactor = 1.3f + (InterpSpeed * 0.1f);
	const float CompensatedTargetVelocity = LandingVelocity * SpeedCompensationFactor;
	
	// 초기 보간을 통해 부드럽게 목표 속도로 전환
	float NewVelocityZ = FMath::FInterpTo(CurrentVelocityZ, CompensatedTargetVelocity, DeltaTime, InterpSpeed);
	
	// 보간이 완료된 후에는 일정한 속도 유지 (중력 영향 제거)
	if (ElapsedTime > (1.0f / InterpSpeed))
	{
		NewVelocityZ = LandingVelocity; // 원래 목표 속도로 일정하게 유지
	}
	
	UE_LOG(LogLuxActionSystem, VeryVerbose, TEXT("[%s] 자연착지 속도 제어 - CurrentZ: %.2f, TargetZ: %.2f, CompensatedZ: %.2f, ElapsedTime: %.2f"), 
		*GetNameSafe(this), CurrentVelocityZ, LandingVelocity, CompensatedTargetVelocity, ElapsedTime);
	
	return NewVelocityZ;
}

float ULuxActionTask_LandingControl::CalculateBasicLandingVelocity() const
{
	// 기본 방식: 즉시 일정한 속도로 변경 (중력 영향 없음)
	return LandingVelocity;
}


