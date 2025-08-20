// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/LuxCameraComponent.h"
#include "Camera/LuxCameraMode.h"
#include "Camera/LuxCameraMode_Focus.h"
#include "Targeting/TargetingComponent.h"
#include "Targeting/LuxTargetingInterface.h"
#include "Targeting/Filters/TargetFilter.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UI/LuxHUD.h"
#include "Engine/Canvas.h"

ULuxCameraComponent::ULuxCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULuxCameraComponent::OnRegister()
{
    Super::OnRegister();

    CameraView.Location = GetComponentLocation();
    CameraView.Rotation = GetComponentRotation();
}

void ULuxCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentModeInstance)
    {
        UpdateCamera(DeltaTime);
    }
}

void ULuxCameraComponent::UpdateCamera(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FLuxCameraModeView IdealView;
    if (CurrentModeInstance && CurrentModeInstance->CanActivate())
    {
        CurrentModeInstance->UpdateCamera(this, DeltaTime, IdealView);
    }
    else
    {
        IdealView = CameraView;
    }

    if (CurrentModeInstance && bShowDebug)
    {
        CurrentModeInstance->DrawDebug(this, IdealView);
    }

    // 포커스 모드가 활성화되어 있다면 위에서 계산된 IdealView를 수정합니다.
    if (FocusModeInstance && FocusModeInstance->CanActivate())
    {
        FocusModeInstance->UpdateCamera(this, DeltaTime, IdealView);
    }

    if(bShowFoucsDebug && FocusModeInstance)
    {
        FocusModeInstance->DrawDebug(this, IdealView);
    }

    FVector FinalLocation = IdealView.Location;
    AActor* TargetActor = GetTargetActor();
    if (CurrentModeInstance->bUseLagSubsystem)
    {
        // Lag의 목표 지점(LagTargetLocation)을 카메라의 이상적인 위치로 부드럽게 보간합니다.
        LagTargetLocation = FMath::VInterpTo(LagTargetLocation, IdealView.Location, DeltaTime, CurrentModeInstance->LagSpeed);

        // 최종 목표 위치를 캐릭터가 아닌 Lag 목표 지점으로 교체합니다.
        IdealView.Location = LagTargetLocation;
    }

    // 충돌 방지 기능을 활성화했다면 캐릭터 사이에 장애물이 있는지 확인합니다.
    if (bEnableCollisionPrevention && TargetActor)
    {
        PreventCameraCollision(FinalLocation, TargetActor->GetActorLocation());
    }

    if (bIsBlending)
    {
        UpdateCameraBlending(DeltaTime, IdealView);
    }
    else
    {
        // 위치 보간
        CameraView.Location = FMath::VInterpTo(CameraView.Location, FinalLocation, DeltaTime, LocationLagSpeed);
        // 회전 보간
        CameraView.Rotation = FMath::RInterpTo(CameraView.Rotation, IdealView.Rotation, DeltaTime, RotationLagSpeed);
        // FOV는 즉시 적용
        CameraView.FieldOfView = FMath::FInterpTo(CameraView.FieldOfView, IdealView.FieldOfView, DeltaTime, FOVLagSpeed);
    }

    // 최종 계산된 뷰를 실제 카메라 컴포넌트에 적용합니다.
    SetWorldLocationAndRotation(CameraView.Location, CameraView.Rotation);
    SetFieldOfView(CameraView.FieldOfView);
}

void ULuxCameraComponent::StartCameraBlend(const ULuxCameraMode* NewCamMode)
{
    if (NewCamMode && NewCamMode->BlendTime > 0.0f)
    {
        bIsBlending = true;
        BlendAlpha = 0.0f;
        BlendInitialView = CameraView; // 현재 뷰에서 시작
        BlendDuration = NewCamMode->BlendTime;
        BlendFunction = NewCamMode->BlendFunction;
        BlendExp = NewCamMode->BlendExp;
    }
    else
    {
        // 블렌드 시간이 없으면 즉시 전환을 위해 블렌딩을 비활성화합니다.
        bIsBlending = false;
    }
}

void ULuxCameraComponent::UpdateCameraBlending(float DeltaTime, const FLuxCameraModeView& TargetView)
{
    BlendTargetView = TargetView;

    if (BlendDuration > 0.0f)
    {
        BlendAlpha += (DeltaTime / BlendDuration);
    }
    else
    {
        BlendAlpha = 1.0f;
    }

    float EasedAlpha = BlendAlpha;
    switch (BlendFunction)
    {
    case VTBlend_Cubic:
        EasedAlpha = FMath::CubicInterp(0.f, 1.f, 0.f, 0.f, BlendAlpha);
        break;
    case VTBlend_EaseIn:
        EasedAlpha = FMath::InterpEaseIn(0.f, 1.f, BlendAlpha, BlendExp);
        break;
    case VTBlend_EaseOut:
        EasedAlpha = FMath::InterpEaseOut(0.f, 1.f, BlendAlpha, BlendExp);
        break;
    case VTBlend_EaseInOut:
        EasedAlpha = FMath::InterpEaseInOut(0.f, 1.f, BlendAlpha, BlendExp);
        break;
    case VTBlend_Linear:
    default:
        // Linear는 별도 처리 없음
        break;
    }

    // EasedAlpha 값에 따라 부드럽게 보간
    CameraView.Location = FMath::Lerp(BlendInitialView.Location, BlendTargetView.Location, EasedAlpha);
    CameraView.Rotation = FMath::Lerp(BlendInitialView.Rotation, BlendTargetView.Rotation, EasedAlpha);
    CameraView.FieldOfView = FMath::Lerp(BlendInitialView.FieldOfView, BlendTargetView.FieldOfView, EasedAlpha);

    if (BlendAlpha >= 1.0f)
    {
        bIsBlending = false;
    }
}


void ULuxCameraComponent::PushCameraMode(TSubclassOf<ULuxCameraMode> CameraModeClass)
{
    if (!CameraModeClass) return;

    TSubclassOf<ULuxCameraMode> OldModeClass = GetCurrentCameraMode();
    CameraModeStack.Add(CameraModeClass);

    // 이전 모드 인스턴스가 있다면 비활성화
    if (CurrentModeInstance)
    {
        CurrentModeInstance->OnDeactivation(this);
    }

    CurrentModeInstance = NewObject<ULuxCameraMode>(this, CameraModeClass);
    CurrentModeInstance->OnActivation(this);

    const ULuxCameraMode* ModeCDO = GetDefault<ULuxCameraMode>(CameraModeClass);
    StartCameraBlend(ModeCDO);

    if (ModeCDO && ModeCDO->bUseLagSubsystem)
    {
        LagTargetLocation = CameraView.Location;
    }

    // 카메라 모드의 타겟팅 필터들을 적용
    ApplyTargetingFilters(ModeCDO);

    if (ModeCDO && ModeCDO->bEnableFocus)
    {
        // bUseFocusMode가 true이면, 포커스 모드를 활성화합니다.
        EnableFocusMode(ModeCDO->FocusModeToUse);
    }
    else
    {
        DisableFocusMode();
    }
  
    OnCameraModeChanged.Broadcast(OldModeClass, CameraModeClass);
}

void ULuxCameraComponent::PopCameraMode()
{
    // 스택에 모드가 하나만 남았거나 비어있으면 Pop하지 않습니다. (최소 하나는 유지)
    if (CameraModeStack.Num() <= 1)
    {
        return;
    }

    TSubclassOf<ULuxCameraMode> OldModeClass = GetCurrentCameraMode();

    // 현재 모드 비활성화 및 스택에서 제거
    if (CurrentModeInstance)
    {
        CurrentModeInstance->OnDeactivation(this);
    }
    CameraModeStack.Pop();

    // 이전 모드를 다시 활성화
    TSubclassOf<ULuxCameraMode> NewModeClass = GetCurrentCameraMode();
    CurrentModeInstance = NewObject<ULuxCameraMode>(this, NewModeClass);
    CurrentModeInstance->OnActivation(this);


    const ULuxCameraMode* ModeCDO = GetDefault<ULuxCameraMode>(NewModeClass);
    StartCameraBlend(ModeCDO);

    if (ModeCDO && ModeCDO->bUseLagSubsystem)
    {
        LagTargetLocation = CameraView.Location;
    }

    // 카메라 모드의 타겟팅 필터들을 적용
    ApplyTargetingFilters(ModeCDO);

    if (ModeCDO && ModeCDO->FocusModeToUse)
    {
        EnableFocusMode(ModeCDO->FocusModeToUse);
    }
    else
    {
        DisableFocusMode();
    }

    OnCameraModeChanged.Broadcast(OldModeClass, NewModeClass);
}

void ULuxCameraComponent::EnableFocusMode(TSubclassOf<ULuxCameraMode_Focus> FocusModeClass)
{
    if (!FocusModeClass) return;

    if (FocusModeInstance)
    {
        FocusModeInstance->OnDeactivation(this);
    }

    FocusModeInstance = NewObject<ULuxCameraMode_Focus>(this, FocusModeClass);
    FocusModeInstance->OnActivation(this);
}

void ULuxCameraComponent::DisableFocusMode()
{
    if (FocusModeInstance)
    {
        FocusModeInstance->OnDeactivation(this);
        FocusModeInstance = nullptr;
    }
}

TSubclassOf<ULuxCameraMode> ULuxCameraComponent::GetCurrentCameraMode() const
{
    if (CameraModeStack.IsEmpty())
    {
        return nullptr;
    }
    return CameraModeStack.Last();
}

AActor* ULuxCameraComponent::GetTargetActor() const
{
    return GetOwner();
}

void ULuxCameraComponent::PreventCameraCollision(FVector& InOutLocation, const FVector& Origin)
{
    FHitResult HitResult;
    const FVector Start = Origin;
    const FVector End = InOutLocation;

    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(GetTargetActor());

    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeSphere(CollisionProbeSize),
        CollisionParams
    );

    // 장애물과 충돌했다면 충돌 위치를 새로운 최종 위치로 설정
    if (bHit)
    {
        InOutLocation = HitResult.Location;
    }
}

UTargetingComponent* ULuxCameraComponent::FindTargetingComponent() const
{
    AActor* TargetActor = GetTargetActor();
    if (!TargetActor)
    {
        return nullptr;
    }

    // 먼저 타겟팅 인터페이스를 통해 찾기
    ILuxTargetingInterface* TargetingInterface = Cast<ILuxTargetingInterface>(TargetActor);
    if (TargetingInterface)
    {
        return TargetingInterface->GetTargetingComponent();
    }

    // 인터페이스가 없으면 직접 컴포넌트 찾기
    return TargetActor->FindComponentByClass<UTargetingComponent>();
}

void ULuxCameraComponent::ApplyTargetingFilters(const ULuxCameraMode* CameraMode)
{
    if (!CameraMode)
    {
        return;
    }

    UTargetingComponent* TargetingComponent = FindTargetingComponent();
    if (!TargetingComponent)
    {
        return;
    }

    // 기존 기본 필터들을 지우고 새로운 필터들을 설정
    TArray<UTargetFilter*> FilterInstances;

    // 카메라 모드에서 정의된 필터 클래스들을 인스턴스로 생성
    for (TSubclassOf<UTargetFilter> FilterClass : CameraMode->TargetingFilters)
    {
        if (FilterClass)
        {
            UTargetFilter* FilterInstance = NewObject<UTargetFilter>(this, FilterClass);
            if (FilterInstance)
            {
                FilterInstances.Add(FilterInstance);
            }
        }
    }

    // 타겟팅 컴포넌트에 필터들을 설정
    TargetingComponent->SetDefaultTargetingFilters(FilterInstances);
}