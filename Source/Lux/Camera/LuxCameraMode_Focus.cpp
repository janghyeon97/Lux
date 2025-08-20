// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/LuxCameraMode_Focus.h"
#include "Camera/LuxCameraComponent.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "Targeting/TargetingComponent.h"
#include "Targeting/LuxTargetingInterface.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "LuxLogChannels.h"

ULuxCameraMode_Focus::ULuxCameraMode_Focus()
{
	PreviousControlRotation = FRotator::ZeroRotator;
}

void ULuxCameraMode_Focus::OnActivation(ULuxCameraComponent* CameraComponent)
{
	Super::OnActivation(CameraComponent);

	// 타겟팅 컴포넌트 설정
	ConfigureTargetingComponent();
}

void ULuxCameraMode_Focus::OnDeactivation(ULuxCameraComponent* CameraComponent)
{
	Super::OnDeactivation(CameraComponent);

	// 타겟팅 컴포넌트의 DetectionType 오버라이드 해제
	if (TargetingComponent)
	{
		TargetingComponent->ClearDetectionTypeOverride();
	}
}

void ULuxCameraMode_Focus::UpdateCamera(ULuxCameraComponent* CameraComponent, float DeltaTime, FLuxCameraModeView& InOutView)
{
	// TargetingComponent가 없으면 설정
	if (!TargetingComponent)
	{
		ConfigureTargetingComponent();
	}

	// TargetingComponent에서 이미 계산된 타겟을 가져오기
	if (TargetingComponent)
	{
		FocusTarget = TargetingComponent->GetCurrentTarget();
	}

	APawn* AvatarActor = Cast<APawn>(GetTargetActor());
	AController* AvatarController = AvatarActor ? AvatarActor->GetController() : nullptr;

	if (FocusTarget && AvatarActor && AvatarController)
	{
		const FVector TargetLocation = FocusTarget->GetActorLocation();
		
		// 이미 계산된 카메라 위치에서 타겟을 바라보는 방향으로 LookAtRotation 계산
		const FRotator LookAtRotation = FRotationMatrix::MakeFromX(TargetLocation - InOutView.Location).Rotator();

		FRotator CurrentControlRotation = AvatarController->GetControlRotation();
		FRotator DesiredRotation = CurrentControlRotation;
		DesiredRotation.Yaw = LookAtRotation.Yaw;

		// PreviousControlRotation이 초기화되지 않은 경우 현재 회전값으로 설정
		if (PreviousControlRotation.IsZero())
		{
			PreviousControlRotation = CurrentControlRotation;
		}

		// 부드러운 회전 보간
		FRotator NewControlRotation = FMath::RInterpTo(CurrentControlRotation, DesiredRotation, DeltaTime, RotationSpeed);
		AvatarController->SetControlRotation(NewControlRotation);
		
		// 현재 회전값을 다음 프레임을 위해 저장
		PreviousControlRotation = NewControlRotation;
		
		// 최종 회전값을 InOutView에 적용 (위치는 부모 클래스에서 이미 계산됨)
		InOutView.Rotation = DesiredRotation;
	}
}

void ULuxCameraMode_Focus::ConfigureTargetingComponent()
{
	if (!TargetingComponent)
	{
		// 타겟팅 컴포넌트 찾기
		TargetingComponent = FindTargetingComponent();
	}

	if (!TargetingComponent)
	{
		return;
	}

	// 이미 설정된 DetectionType과 다를 때만 설정 (불필요한 오버라이드 방지)
	if (TargetingComponent->GetCurrentDetectionType() != DetectionType)
	{
		UE_LOG(LogLux, Log, TEXT("[%s] DetectionType 설정 및 서버 동기화 요청: %d"), 
			*GetName(), (int32)DetectionType);
			
		// 이 함수가 자동으로 RPC를 호출하여 서버와 동기화
		TargetingComponent->SetDetectionType(DetectionType);
	}

	// 거리 설정
	if (DetectionType == ETargetingDetectionType::Overlap)
	{
		TargetingComponent->SetOverlapRadius(OverlapRadius);
	}
	else
	{
		TargetingComponent->SetTraceDistance(TraceDistance);
	}

	// SweepRadius 설정
	if (DetectionType == ETargetingDetectionType::SweepTrace)
	{
		TargetingComponent->SetSweepRadius(SweepRadius);
	}

	// 우선순위 가중치 설정
	if (bEnablePrioritySelection)
	{
		TargetingComponent->SetPriorityWeights(DistanceWeight, AngleWeight, SizeWeight);
	}
}

UTargetingComponent* ULuxCameraMode_Focus::FindTargetingComponent()
{
	AActor* TargetActor = GetTargetActor();
	if (!TargetActor)
	{
		return nullptr;
	}

	ILuxTargetingInterface* TagetingInterface = Cast<ILuxTargetingInterface>(TargetActor);
	if (TagetingInterface)
	{
		return TagetingInterface->GetTargetingComponent();
	}

	return TargetActor->FindComponentByClass<UTargetingComponent>();
}

void ULuxCameraMode_Focus::ClearFocus()
{
	// TargetingComponent의 타겟도 함께 해제
	if (TargetingComponent)
	{
		TargetingComponent->ClearTarget();
	}
	
	FocusTarget = nullptr;
}

void ULuxCameraMode_Focus::ForceClearFocus()
{
	// 사용자가 의도적으로 포커스를 해제
	// 이전 회전 정보도 초기화하여 카메라 복원 트리거
	PreviousControlRotation = FRotator::ZeroRotator;
	
	// TargetingComponent의 타겟도 함께 해제
	if (TargetingComponent)
	{
		TargetingComponent->ClearTarget();
	}
	
	ClearFocus();
}

void ULuxCameraMode_Focus::DrawDebug(const ULuxCameraComponent* CameraComponent, const FLuxCameraModeView& InView) const
{
	if (!FocusTarget) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FString DebugText = FString::Printf(TEXT("Focus Target: %s\nLocation: %s"),
		*FocusTarget->GetName(),
		*FocusTarget->GetActorLocation().ToCompactString());

	::DrawDebugSphere(CameraComponent->GetWorld(), FocusTarget->GetActorLocation(), 30.f, 16, FColor::Magenta, false, 0.f, 0, 2.f);
	::DrawDebugString(CameraComponent->GetWorld(), FocusTarget->GetActorLocation() + FVector(0, 0, 50.f), DebugText, nullptr, FColor::Magenta, 0.f, true);

	// 카메라 위로 살짝 올린 위치에서 화살표 시작
	const FVector UpOffset = FVector(0, 0, 60.f); // 60cm 위
	const FVector Start = InView.Location + UpOffset;
	const FVector End = Start + InView.Rotation.Vector() * 80.f;

	::DrawDebugDirectionalArrow(World, Start, End, 20.f, FColor::Red, false, 0.f, 0, 2.f);
}