// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/LuxCameraMode.h"
#include "Targeting/TargetingComponent.h"
#include "LuxCameraMode_Focus.generated.h"

/**
 * 주변의 타겟을 자동으로 감지하여 카메라가 해당 타겟을 바라보도록 회전만 수정하는 특수 카메라 모드입니다.
 * UTargetingComponent를 사용하여 타겟을 검출하고, 카메라는 해당 타겟을 향해 회전합니다.
 */
UCLASS(Blueprintable)
class LUX_API ULuxCameraMode_Focus : public ULuxCameraMode
{
    GENERATED_BODY()

public:
    ULuxCameraMode_Focus();

    // ~ ULuxCameraMode overrides
    virtual void OnActivation(ULuxCameraComponent* CameraComponent) override;
    virtual void OnDeactivation(ULuxCameraComponent* CameraComponent) override;
    virtual void UpdateCamera(ULuxCameraComponent* CameraComponent, float DeltaTime, FLuxCameraModeView& InOutView) override;
    virtual void DrawDebug(const ULuxCameraComponent* CameraComponent, const FLuxCameraModeView& InView) const override;

    UFUNCTION(BlueprintPure, Category = "Focus")
    AActor* GetCurrentTarget() const { return FocusTarget; }

    /** 포커스 모드를 해제합니다. */
    UFUNCTION(BlueprintCallable, Category = "Focus")
    void ClearFocus();

    /** 포커스가 활성화되어 있는지 확인합니다. */
    UFUNCTION(BlueprintPure, Category = "Focus")
    bool IsFocusActive() const { return FocusTarget != nullptr; }

    /** 사용자가 의도적으로 포커스를 해제합니다. (화면 각도와 관계없이) */
    UFUNCTION(BlueprintCallable, Category = "Focus")
    void ForceClearFocus();

protected:
    /** UTargetingComponent에 타겟팅 설정을 적용합니다. */
    void ConfigureTargetingComponent();

    /** 타겟팅 컴포넌트를 찾습니다. */
    UTargetingComponent* FindTargetingComponent();

protected:
    /** 타겟을 검출할 방식을 선택합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings")
    ETargetingDetectionType DetectionType = ETargetingDetectionType::Overlap;

    /** 타겟을 향해 카메라가 회전하는 속도입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings")
    float RotationSpeed = 2.0f;

    /** 타겟 검출 업데이트 간격 (초) - 카메라 흔들림 방지용 */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float TargetUpdateInterval = 0.2f;

    /** Overlap 방식의 감지 반경입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings", meta = (EditCondition = "DetectionType == ETargetingDetectionType::Overlap"))
    float OverlapRadius = 2500.0f;

    /** Line/Sweep Trace 방식의 감지 거리입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings", meta = (EditCondition = "DetectionType != ETargetingDetectionType::Overlap"))
    float TraceDistance = 5000.0f;

    /** Sweep Trace 방식의 스피어(Sphere) 반지름입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings", meta = (EditCondition = "DetectionType == ETargetingDetectionType::SweepTrace"))
    float SweepRadius = 50.0f;

    /** 거리, 크기, 각도를 종합하여 타겟 우선순위를 계산하는 기능을 활성화합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings")
    bool bEnablePrioritySelection = true;

    /** 거리에 대한 가중치입니다. (가까울수록 높음) */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings", meta = (EditCondition = "bEnablePrioritySelection"))
    float DistanceWeight = 0.5f;

    /** 화면 중앙과의 각도에 대한 가중치입니다. (중앙에 가까울수록 높음) */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings", meta = (EditCondition = "bEnablePrioritySelection"))
    float AngleWeight = 0.3f;

    /** 크기에 대한 가중치입니다. (클수록 높음) */
    UPROPERTY(EditDefaultsOnly, Category = "Focus|Settings", meta = (EditCondition = "bEnablePrioritySelection"))
    float SizeWeight = 0.2f;

private:
    UPROPERTY(Transient)
    TObjectPtr<AActor> FocusTarget;

    /** 이전 컨트롤 회전값 (부드러운 전환용) */
    UPROPERTY(Transient)
    FRotator PreviousControlRotation;

    /** 타겟팅 컴포넌트에 대한 참조입니다. */
    UPROPERTY(Transient)
    TObjectPtr<UTargetingComponent> TargetingComponent;
};