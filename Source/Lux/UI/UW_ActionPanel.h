// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/LuxUserWidgetBase.h"
#include "UW_ActionPanel.generated.h"


class UBorder; 
class UUW_ActionIcon;
class UMainHUD;

/**
 * 캐릭터가 보유한 액션(스킬)들을 UI에 표시하는 패널 위젯입니다.
 * 각 액션은 InputTag에 따라 지정된 슬롯에 배치됩니다.
 */
UCLASS()
class LUX_API UUW_ActionPanel : public ULuxUserWidgetBase
{
	GENERATED_BODY()
	
public:
    // ~ ULuxUserWidgetBase overrides
    virtual void InitializeWidget(UActionSystemComponent* InASC) override;

    /** MainHUD 참조를 받아서 각 ActionIcon에 설정합니다. */
    void SetMainHUDReference(UMainHUD* InMainHUD);

protected:
	// ~ UUserWidget overrides
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

    /** ASC에서 액션 목록을 가져와 각 슬롯에 맞는 아이콘을 생성하고 배치합니다. */
    void RefreshActionIcons();

protected:
    /** 주 공격(마우스 좌클릭 등) 스킬 아이콘이 들어갈 슬롯입니다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> PrimaryActionSlot;

    /** 보조(마우스 우클릭 등) 스킬 아이콘이 들어갈 슬롯입니다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> SecondaryActionSlot;

    /** Q 스킬 아이콘이 들어갈 슬롯입니다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> TertiaryActionSlot;

    /** E 스킬 아이콘이 들어갈 슬롯입니다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> QuaternaryActionSlot;

    /** R (궁극기) 스킬 아이콘이 들어갈 슬롯입니다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> UltimateActionSlot;

private:
    /** 이 패널에서 생성할 개별 스킬 아이콘 위젯의 블루프린트트 클래스입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUW_ActionIcon> ActionIconWidgetClass;

    /** 이 패널에서 생성할 개별 스킬 아이콘 위젯의 인스턴스입니다. */
    UPROPERTY()
    TArray<TObjectPtr<UUW_ActionIcon>> ActionIcons;
};
