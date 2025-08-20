// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/LuxUserWidgetBase.h"
#include "MainHUD.generated.h"


class UVitalsWidget;
class UUW_ActionPanel;
class UActionSystemComponent;
class UUW_ActionTooltip;
class USizeBox;


/**
 * 
 */
UCLASS()
class LUX_API UMainHUD : public ULuxUserWidgetBase
{
	GENERATED_BODY()
	
public:
    UMainHUD(const FObjectInitializer& ObjectInitializer);

    // ~ ULuxUserWidgetBase overrides
    virtual void InitializeWidget(UActionSystemComponent* InASC) override;

    /** 툴팁을 표시합니다. */
    void ShowTooltip(UUW_ActionTooltip* TooltipWidget);

    /** 툴팁을 숨깁니다. */
    void HideTooltip();

protected:
    // ~ UUserWidget overrides
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    /** 캐릭터의 HP, MP 등 핵심 자원을 표시하는 위젯입니다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainHUD", Meta = (BindWidget))
    TObjectPtr<UVitalsWidget> VitalsWidget;

    /** 캐릭터의 액션 아이콘들을 표시하는 위젯입니다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainHUD", Meta = (BindWidget))
    TObjectPtr<UUW_ActionPanel> ActionPanel;

    /** 툴팁을 담을 SizeBox (블루프린트에서 크기와 위치 설정) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tooltip", Meta = (BindWidget))
    TObjectPtr<USizeBox> TooltipSizeBox;
};
