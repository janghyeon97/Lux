// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/LuxUserWidgetBase.h"
#include "ActionSystem/Actions/LuxActionTypes.h"
#include "UW_ActionIcon.generated.h"

class UScaleBox;
class UImage;
class UTexture;
class UTextBlock;
class UMaterialInstanceDynamic;
class UActionSystemComponent;
class UUW_ActionTooltip;
class UMainHUD;
class ULuxCooldownTracker;

/**
 * 단일 액션(스킬)의 아이콘과 쿨다운 상태를 표시하는 위젯입니다.
 */
UCLASS()
class LUX_API UUW_ActionIcon : public ULuxUserWidgetBase
{
    GENERATED_BODY()

    friend class UUW_ActionPanel;

public:
    // ~ ULuxUserWidgetBase overrides
    virtual void InitializeWidget(UActionSystemComponent* InASC) override;

    /** 아이콘 위젯을 특정 Action Spec의 데이터로 초기화합니다. */
    virtual void InitializeWidget(UActionSystemComponent* InASC, const FLuxActionSpec& InSpec);

    void SetIcon(UTexture* NewIcon);
    void SetCooldownPercent(float Percent);

protected:
    // ~ UUserWidget overrides
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    /** CooldownTracker에서 쿨다운이 시작될 때 호출되는 콜백 함수입니다. */
    UFUNCTION()
    void OnCooldownStarted(const FGameplayTag& CooldownTag, float Duration);

    /** CooldownTracker에서 쿨다운 진행률이 변경될 때 호출되는 콜백 함수입니다. */
    UFUNCTION()
    void OnCooldownProgressChanged(const FGameplayTag& CooldownTag, float TimeRemaining, float Duration);

    	/** CooldownTracker에서 쿨다운이 종료될 때 호출되는 콜백 함수입니다. */
	UFUNCTION()
	void OnCooldownEnded(const FGameplayTag& CooldownTag);

	/** 실시간 쿨다운 UI 업데이트 함수 (타이머에 의해 호출) */
	UFUNCTION()
	void UpdateCooldownUI();

	/** MainHUD 참조를 설정합니다. */
	void SetMainHUDReference(UMainHUD* InMainHUD);

protected:
    UPROPERTY()
    TObjectPtr<UScaleBox> ScaleBox;

    /** 스킬 아이콘을 표시하는 메인 이미지입니다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ImageIcon;

    /** 남은 쿨다운 시간을 표시하는 텍스트입니다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CooldownText;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    float UpdateInterval = 0.016f;

    /** 툴팁 위젯 클래스 */
    UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
    TSubclassOf<UUW_ActionTooltip> TooltipWidgetClass;

private:
    /** 아이콘이 나타내는 액션의 핸들입니다. */
    UPROPERTY()
    FLuxActionSpecHandle ActionSpecHandle;

    /** 이 액션의 쿨다운을 나타내는 게임플레이 태그입니다. */
    UPROPERTY()
    FGameplayTag CooldownTag;

    /** 쿨다운 오버레이 이미지에 적용된 다이나믹 머티리얼 인스턴스입니다. */
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> CooldownMaterialDynamic;

    /** 쿨다운 시간을 업데이트하기 위한 타이머 핸들입니다. */
    FTimerHandle CooldownTimerHandle;

    /** 현재 표시 중인 툴팁 위젯 인스턴스 */
    UPROPERTY()
    TObjectPtr<UUW_ActionTooltip> CurrentTooltip;

    /** MainHUD 참조 (툴팁 표시용) */
    TWeakObjectPtr<UMainHUD> MainHUD;

    /** 쿨다운 트래커 참조 */
    TWeakObjectPtr<ULuxCooldownTracker> CooldownTracker;

    /** 현재 액션 스펙 (툴팁 생성용) */
    UPROPERTY()
    FLuxActionSpec CurrentActionSpec;

    UPROPERTY(VisibleDefaultsOnly, Transient, meta = (BindWidgetAnim))
    TObjectPtr<class UWidgetAnimation> ReductionAnimation;
};