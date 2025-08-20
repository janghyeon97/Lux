// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/LuxCameraMode.h"
#include "Camera/LuxCameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "Targeting/Filters/TargetFilter.h"

ULuxCameraMode::ULuxCameraMode()
{

}

bool ULuxCameraMode::CanActivate() const
{
    AActor* TargetActor = GetTargetActor();
    if (!TargetActor) return false;

    const UActionSystemComponent* ASC = nullptr;
    if (const IActionSystemInterface* ASI = Cast<IActionSystemInterface>(TargetActor))
    {
        ASC = ASI->GetActionSystemComponent();
    }
    else
    {
        ASC = TargetActor->FindComponentByClass<UActionSystemComponent>();
    }

    // ASC가 없으면 조건을 확인할 수 없으므로 기본적으로 활성화되지 않습니다.
    if (!ASC)
    {
        return false;
    }

    // 차단 태그가 하나라도 있는지 확인합니다.
    if (ASC->HasAny(ActivationBlockedTags))
    {
        return false;
    }

    // 필요한 태그를 모두 가지고 있는지 확인합니다.
    if (!ASC->HasAll(ActivationRequiredTags))
    {
        return false;
    }

    // 모든 조건을 통과했으면 활성화 가능합니다.
    return true;
}

void ULuxCameraMode::OnActivation(ULuxCameraComponent* CameraComponent)
{
    LuxCameraComponent = CameraComponent;
}

void ULuxCameraMode::OnDeactivation(ULuxCameraComponent* CameraComponent)
{
    LuxCameraComponent = nullptr;
}

void ULuxCameraMode::UpdateCamera(ULuxCameraComponent* CameraComponent, float DeltaTime, FLuxCameraModeView& OutView)
{
    const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor());
    if (!TargetCharacter)
    {
        return;
    }

    // 2. 컨트롤러의 회전 값을 가져옵니다.
    FRotator ControlRotation = TargetCharacter->GetControlRotation();

    // 회전 제한 기능이 활성화되었다면 Pitch 각도를 제한합니다.
    if (bEnableRotationLimits)
    {
        ControlRotation.Pitch = FMath::Clamp(ControlRotation.Pitch, MinPitchAngle, MaxPitchAngle);
    }

    FVector IdealLocation;
    FRotator IdealRotation = ControlRotation;

    if (AttachToSocketName != NAME_None && TargetCharacter->GetMesh())
    {
        // AttachToSocketName이 지정된 경우:
        // 해당 소켓의 위치와 회전을 직접 사용하여 정밀한 1인칭 또는 부착 뷰를 구현합니다.
        IdealLocation = TargetCharacter->GetMesh()->GetSocketLocation(AttachToSocketName);
        IdealRotation = TargetCharacter->GetMesh()->GetSocketRotation(AttachToSocketName);
    }
    else
    {
        // AttachToSocketName이 지정되지 않은 경우:
        // ArmLength와 오프셋을 사용한 3인칭 로직을 실행합니다.
        const FVector TargetLocation = TargetCharacter->GetActorLocation();
        IdealLocation = TargetLocation - (ControlRotation.Vector() * ArmLength);
        IdealLocation += ControlRotation.RotateVector(LocationOffset);
    }

    View.Location = IdealLocation;
    View.Rotation = IdealRotation;

    // 동적 FOV 기능이 활성화되었다면 속도에 따라 시야각을 조절합니다.
    if (bEnableDynamicFOV)
    {
        const float Speed = TargetCharacter->GetVelocity().Size();
        const float SpeedRatio = FMath::GetMappedRangeValueClamped(FVector2D(0, DynamicFOVSpeedThreshold), FVector2D(0, 1), Speed);
        const float TargetFOV = FMath::Lerp(FieldOfView, DynamicFOVMax, SpeedRatio);

        CurrentDynamicFOV = FMath::FInterpTo(CurrentDynamicFOV, TargetFOV, DeltaTime, DynamicFOVInterpSpeed);
        View.FieldOfView = CurrentDynamicFOV;
    }
    else
    {
        View.FieldOfView = FieldOfView;
    }

    // 최종 계산된 View 데이터를 결과로 반환합니다.
    OutView = View;
}

AActor* ULuxCameraMode::GetTargetActor() const
{
    if (LuxCameraComponent)
    {
        return LuxCameraComponent->GetTargetActor();
    }

    return nullptr;
}

void ULuxCameraMode::DrawDebug(const ULuxCameraComponent* CameraComponent, const FLuxCameraModeView& InView) const
{
    if (!CameraComponent || !CameraComponent->GetWorld())
    {
        return;
    }

    UWorld* World = CameraComponent->GetWorld();

    // 1. 이 모드가 계산한 View의 위치와 회전을 기준으로 트레이스를 시작합니다.
    const FVector TraceStart = InView.Location;
    const FVector TraceEnd = TraceStart + (InView.Rotation.Vector() * 2000.f);

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    if (AActor* TargetActor = GetTargetActor())
    {
        CollisionParams.AddIgnoredActor(TargetActor);
    }
    // 카메라 컴포넌트의 오너도 무시 리스트에 추가합니다.
    if (AActor* Owner = CameraComponent->GetOwner())
    {
        CollisionParams.AddIgnoredActor(Owner);
    }

    const bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, CollisionParams);

    // 2. 충돌 지점 또는 최대 사거리에 구체를 그립니다.
    const FVector HitLocation = bHit ? HitResult.Location : TraceEnd;

    // 파란색 구와 선으로 결과를 표시합니다.
    ::DrawDebugSphere(World, HitLocation, 16.f, 12, FColor::Blue, false, 0.f, 0, 1.f);
    ::DrawDebugString(World, HitLocation, TEXT("Mode Hit"), nullptr, FColor::Blue, 0.f, true);

     // 카메라 위로 살짝 올린 위치에서 화살표 시작
     const FVector UpOffset = FVector(0, 0, 60.f); // 60cm 위
     const FVector Start = InView.Location + UpOffset;
     const FVector End = Start + InView.Rotation.Vector() * 80.f; // 80cm 길이
 
     ::DrawDebugDirectionalArrow(World, Start, End, 20.f, FColor::Blue, false, 0.f, 0, 2.f);
}