// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "LuxCameraMode.generated.h"

class UTargetFilter;
class ULuxCameraComponent;
class ULuxCameraMode_Focus;


/**
 * 카메라의 최종 위치, 회전, FOV 등 뷰(View) 데이터를 담는 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FLuxCameraModeView
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "View")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "View")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "View")
    float FieldOfView = 90.0f;
};

/**
 * 모든 카메라 모드의 기반이 되는 클래스입니다.
 * 기본적으로 완전한 3인칭 카메라 기능을 제공하며,
 * 하위 클래스에서 프로퍼티를 수정하거나 함수를 오버라이드하여 새로운 모드를 만들 수 있습니다.
 */
UCLASS(Blueprintable)
class LUX_API ULuxCameraMode : public UObject
{
    GENERATED_BODY()
    
    friend class ALuxHUD;
    friend class ULuxCameraComponent;

public:
    ULuxCameraMode();

    /** 카메라 모드가 활성화될 때 호출됩니다. */
    virtual void OnActivation(ULuxCameraComponent* CameraComponent);

    /** 카메라 모드가 비활성화될 때 호출됩니다. */
    virtual void OnDeactivation(ULuxCameraComponent* CameraComponent);

    /** 매 틱 호출되어 모드의 카메라 뷰를 계산합니다. */
    virtual void UpdateCamera(ULuxCameraComponent* CameraComponent, float DeltaTime, FLuxCameraModeView& OutView);

    /** 카메라 모드를 활성화될 수 있는 지 확인합니다. */
    virtual bool CanActivate() const;

    /** 이 모드에 대한 디버그 정보를 그립니다. */
    virtual void DrawDebug(const ULuxCameraComponent* CameraComponent, const FLuxCameraModeView& InView) const;

protected:
    /** 타겟 액터를 반환합니다. 보통의 경우 플레이어 캐릭터입니다. */
    AActor* GetTargetActor() const;

protected:
    /** 이 카메라 모드에서 사용할 타겟팅 필터들입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting|Filters")
    TArray<TSubclassOf<UTargetFilter>> TargetingFilters;

    /** 이 카메라 모드에서 포커스 기능을 활성화합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Focus")
    bool bEnableFocus = false;

    /** bUseFocusMode가 true일 때 활성화할 포커스 모드의 클래스입니다.*/
    UPROPERTY(EditDefaultsOnly, Category = "Focus", meta = (EditCondition = "bEnableFocus"))
    TSubclassOf<ULuxCameraMode_Focus> FocusModeToUse;


    /** 이 카메라 모드가 활성화되었을 때, 전용 랙(Lag) 서브시스템을 사용할지 여부입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Lag")
    bool bUseLagSubsystem = false;

    /** 랙(Lag) 서브시스템이 활성화되었을 때, 카메라가 목표를 따라가는 속도입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Lag", meta = (EditCondition = "bUseLagSubsystem"))
    float LagSpeed = 10.0f;



    /**
      * 타겟의 특정 소켓에 카메라를 부착합니다.
      * 'None'이 아닐 경우 ArmLength나 LocationOffset 대신 이 소켓의 위치를 따릅니다.
      */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode")
    FName AttachToSocketName = NAME_None;

    /** 타겟으로부터 카메라가 떨어져 있는 기본 거리입니다. (AttachToSocketName이 'None'일 때만 작동) */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode")
    float ArmLength = 400.0f;

    /** 카메라의 위치 오프셋입니다. (AttachToSocketName이 'None'일 때만 작동) */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode")
    FVector LocationOffset = FVector(0.0f, 0.0, 60.0f);

    

    /** 이 카메라 모드의 기본 시야각(Field of View)입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode")
    float FieldOfView = 90.0f;


    /** 이 카메라 모드로 전환될 때 사용할 블렌드 시간 (초)입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Blending")
    float BlendTime = 0.5f;

    /** 이 카메라 모드로 전환될 때 사용할 블렌드 함수 유형입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Blending")
    TEnumAsByte<EViewTargetBlendFunction> BlendFunction = VTBlend_EaseInOut;

    /** 이 카메라 모드로 전환될 때 블렌딩에 사용할 커브의 강도입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Blending")
    float BlendExp = 2.0f;


    /** 속도에 따라 FOV가 동적으로 변경되는 기능을 활성화합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Dynamic FOV")
    bool bEnableDynamicFOV = false;

    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Dynamic FOV", meta = (EditCondition = "bEnableDynamicFOV", EditConditionHides))
    float DynamicFOVMax = 110.0f;

    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Dynamic FOV", meta = (EditCondition = "bEnableDynamicFOV", EditConditionHides))
    float DynamicFOVSpeedThreshold = 800.0f;

    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Dynamic FOV", meta = (EditCondition = "bEnableDynamicFOV", EditConditionHides))
    float DynamicFOVInterpSpeed = 4.0f;


    /** 카메라의 상하 회전 각도를 제한합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Rotation Limits")
    bool bEnableRotationLimits = true;

    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Rotation Limits", meta = (EditCondition = "bEnableRotationLimits", EditConditionHides))
    float MinPitchAngle = -75.0f;

    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Rotation Limits", meta = (EditCondition = "bEnableRotationLimits", EditConditionHides))
    float MaxPitchAngle = 75.0f;



    /** 이 모드가 활성화되기 위해, 시전자가 반드시 가지고 있어야 하는 모든 태그입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Conditions")
    FGameplayTagContainer ActivationRequiredTags;

    /** 이 중 하나라도 시전자가 가지고 있으면, 이 모드는 활성화될 수 없습니다. */
    UPROPERTY(EditDefaultsOnly, Category = "CameraMode|Conditions")
    FGameplayTagContainer ActivationBlockedTags;

   
protected:
    UPROPERTY()
    TObjectPtr<ULuxCameraComponent> LuxCameraComponent;

    UPROPERTY()
    FLuxCameraModeView View;

    float CurrentDynamicFOV;
};