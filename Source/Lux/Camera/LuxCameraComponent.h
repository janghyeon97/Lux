// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LuxCameraMode.h"
#include "Camera/CameraComponent.h"
#include "LuxCameraComponent.generated.h"

class ULuxCameraMode;
class ULuxCameraMode_Focus;
class UTargetingComponent;
class UTargetFilter;


// 카메라 모드 변경 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCameraModeChanged, TSubclassOf<ULuxCameraMode>, OldMode, TSubclassOf<ULuxCameraMode>, NewMode);


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LUX_API ULuxCameraComponent : public UCameraComponent
{
    GENERATED_BODY()

public:
    ULuxCameraComponent();

    /** 새로운 카메라 모드를 스택의 맨 위에 추가하고 활성화합니다. 이전 모드는 비활성화됩니다. */
    UFUNCTION(BlueprintCallable, Category = "Lux|Camera")
    void PushCameraMode(TSubclassOf<ULuxCameraMode> CameraModeClass);

    /** 스택의 맨 위에 있는 현재 카메라 모드를 제거하고 이전 모드를 다시 활성화합니다. */
    UFUNCTION(BlueprintCallable, Category = "Lux|Camera")
    void PopCameraMode();

    /** 포커스 모드를 활성화합니다. */
    UFUNCTION(BlueprintCallable, Category = "Lux|Camera|Focus")
    void EnableFocusMode(TSubclassOf<ULuxCameraMode_Focus> FocusModeClass);

    /** 포커스 모드를 비활성화합니다. */
    UFUNCTION(BlueprintCallable, Category = "Lux|Camera|Focus")
    void DisableFocusMode();



    /** 현재 활성화된 카메라 모드 클래스를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Lux|Camera")
    TSubclassOf<ULuxCameraMode> GetCurrentCameraMode() const;

    /** 카메라의 타겟이 되는 폰(Pawn)을 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Lux|Camera")
    AActor* GetTargetActor() const;

    /** 현재 활성화된 포커스 모드 인스턴스를 반환합니다. (HUD 디버그용) */
    UFUNCTION(BlueprintPure, Category = "Lux|Camera|Debug")
    ULuxCameraMode_Focus* GetFocusModeInstance() const { return FocusModeInstance; }

    /** 현재 카메라 모드 스택을 반환합니다. (HUD 디버그용) */
    UFUNCTION(BlueprintPure, Category = "Lux|Camera|Debug")
    const TArray<TSubclassOf<ULuxCameraMode>>& GetCameraModeStack() const { return CameraModeStack; }

protected:
    /** 타겟팅 컴포넌트를 찾고 카메라 모드의 필터들을 적용합니다. */
    void ApplyTargetingFilters(const ULuxCameraMode* CameraMode);

    /** 타겟팅 컴포넌트를 찾습니다. */
    UTargetingComponent* FindTargetingComponent() const;

public:
    /** 카메라 모드가 변경될 때 호출되는 이벤트입니다. */
    UPROPERTY(BlueprintAssignable, Category = "Lux|Camera|Events")
    FOnCameraModeChanged OnCameraModeChanged;

protected:
    //~ UActorComponent interface
    virtual void OnRegister() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent interface

protected:
    /** 카메라 뷰를 매 틱 업데이트합니다. */
    void UpdateCamera(float DeltaTime);

    /**
     * 지정된 위치(TargetLocation)와 원점(Origin) 사이의 충돌을 처리합니다.
     * @param InOutLocation - 충돌 검사를 할 위치이며 충돌 시 안전한 위치로 업데이트됩니다.
     * @param Origin - 충돌 검사의 시작점(보통 캐릭터의 위치)입니다.
     */
    void PreventCameraCollision(FVector& InOutLocation, const FVector& Origin);

    /**
    * 새로운 카메라 모드로의 블렌딩을 시작합니다.
    * @param NewCamMode 블렌딩의 목표가 되는 새로운 카메라 모드입니다.
    */
    void StartCameraBlend(const ULuxCameraMode* NewCamMode);

    /**
     * 매 틱 호출되어 카메라 뷰의 블렌딩을 처리합니다.
     * @param DeltaTime 프레임 간의 시간 간격입니다.
     * @param TargetView 블렌딩의 최종 목표 뷰 데이터입니다.
     */
    void UpdateCameraBlending(float DeltaTime, const FLuxCameraModeView& TargetView);

protected:
    /** 목표 지점까지 카메라 위치를 보간하는 속도입니다. */
    UPROPERTY(EditAnywhere, Category = "Lux|Camera Settings")
    float LocationLagSpeed = 15.0f;

    /** 목표 각도까지 카메라 회전을 보간하는 속도입니다. */
    UPROPERTY(EditAnywhere, Category = "Lux|Camera Settings")
    float RotationLagSpeed = 15.0f;

    /** 목표 FOV까지 카메라 시야각을 보간하는 속도입니다. */
    UPROPERTY(EditAnywhere, Category = "Lux|Camera Settings")
    float FOVLagSpeed = 10.0f;


    /** 카메라 충돌 방지 기능을 활성화할지 여부입니다. */
    UPROPERTY(EditAnywhere, Category = "Lux|Camera Settings|Collision")
    bool bEnableCollisionPrevention = true;

    /** 충돌 감지에 사용할 Sphere의 반지름입니다. (0으로 설정 시 Line Trace 사용) */
    UPROPERTY(EditAnywhere, Category = "Lux|Camera Settings|Collision", meta = (EditCondition = "bEnableCollisionPrevention"))
    float CollisionProbeSize = 12.0f;

    /** 충돌 감지에 사용할 트레이스 채널입니다. */
    UPROPERTY(EditAnywhere, Category = "Lux|Camera Settings|Collision", meta = (EditCondition = "bEnableCollisionPrevention"))
    TEnumAsByte<ECollisionChannel> CollisionTraceChannel = ECC_Visibility;


    /** 디버그 정보를 화면에 표시할지 여부입니다. */
    UPROPERTY(EditAnywhere, Category = "Lux|Debug")
    bool bShowDebug = false;

    /** 포커스 디버그 정보를 화면에 표시할지 여부입니다. */
    UPROPERTY(EditAnywhere, Category = "Lux|Debug")
    bool bShowFoucsDebug = false;

private:
    /** 현재 활성화된 카메라 모드 인스턴스입니다. */
    UPROPERTY()
    TObjectPtr<ULuxCameraMode> CurrentModeInstance;

    /** 활성화된 포커스 모드 인스턴스를 담는 전용 슬롯입니다. */
    UPROPERTY()
    TObjectPtr<ULuxCameraMode_Focus> FocusModeInstance;

    /** 카메라 모드 클래스를 저장하는 스택입니다. */
    UPROPERTY()
    TArray<TSubclassOf<ULuxCameraMode>> CameraModeStack;

    /** 카메라의 최종 위치와 회전을 담는 뷰 데이터입니다. */
    UPROPERTY()
    FLuxCameraModeView CameraView;


    /** 랙(Lag)이 적용된 카메라의 목표 위치입니다. */
    FVector LagTargetLocation;


    /** 현재 카메라 모드 블렌딩 중인지 여부입니다. */
    bool bIsBlending = false;

    /** 블렌딩 시작 시점의 뷰 데이터입니다. */
    FLuxCameraModeView BlendInitialView;

    /** 블렌딩 목표 시점의 뷰 데이터입니다. */
    FLuxCameraModeView BlendTargetView;

    /** 현재 블렌딩이 얼마나 진행되었는지를 나타내는 알파 값 (0.0 ~ 1.0) 입니다. */
    float BlendAlpha = 0.0f;

    /** 현재 블렌딩에 걸리는 총 시간입니다. */
    float BlendDuration = 0.0f;

    /** 현재 블렌딩에 사용할 함수 유형입니다. */
    EViewTargetBlendFunction BlendFunction = VTBlend_Linear;

    /** 현재 블렌딩에 사용할 커브의 강도입니다. */
    float BlendExp = 0.0f;
};