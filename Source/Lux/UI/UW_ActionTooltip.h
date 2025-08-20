// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/LuxUserWidgetBase.h"
#include "ActionSystem/Actions/LuxActionTypes.h"
#include "System/ExpressionEvaluator.h"
#include "UW_ActionTooltip.generated.h"

class UActionSystemComponent;
class UTextBlock;
class URichTextBlock;
class UImage;
class UVerticalBox;
class UBorder;
class ULuxAttributeSet;
class UStatMappingData;


/**
 * 액션 아이콘에 마우스를 올렸을 때 표시되는 툴팁 위젯
 * 액션의 이름, 설명, 수치 등을 표시하며 수식 평가를 지원합니다.
 */
UCLASS()
class LUX_API UUW_ActionTooltip : public ULuxUserWidgetBase
{
	GENERATED_BODY()

    friend class UUW_ActionIcon;

public:
    UUW_ActionTooltip(const FObjectInitializer& ObjectInitializer);

    /** 툴팁을 초기화합니다. */
    void InitializeTooltip(UActionSystemComponent* InASC, const FLuxActionSpec& ActionSpec);

    /** 툴팁 데이터를 업데이트합니다. */
    void UpdateTooltipData();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** 액션 설명문에서 수식을 평가하고 동적 수치를 계산합니다. */
    FString EvaluateDescriptionText(const FString& DescriptionText) const;

    /** 텍스트에서 {StatName} 패턴을 찾아 캐릭터 스탯 값으로 치환합니다. */
    FString ReplaceStatReferences(const FString& Text) const;

    /** 텍스트에서 @ActionData@ 패턴을 찾아 액션 레벨 데이터 값으로 치환합니다. */
    FString ReplaceActionDataReferences(const FString& Text) const;

    /** 텍스트에서 [Expression] 패턴을 찾아 수식을 계산한 결과로 치환합니다. */
    FString EvaluateExpressions(const FString& Text) const;

    /** 플레이어의 현재 스탯 값을 가져옵니다. */
    float GetPlayerStatValue(const FString& StatName) const;

    /** 액션의 레벨 데이터 값을 가져옵니다. */
    float GetActionLevelData(const FString& DataName) const;

    /** FInstancedStruct에서 특정 필드의 값을 추출합니다. */
    float ExtractValueFromInstancedStruct(const FInstancedStruct& InstancedStruct, const FString& FieldName) const;

    /** 아이콘을 제외한 텍스트 정보만 업데이트합니다. */
    void UpdateTooltipText(const FLuxActionSpec& ActionSpec);

    /** 아이콘만 업데이트합니다. */
    void UpdateTooltipIcon(UTexture2D* IconTexture);

protected:
    /** 액션 이름을 표시하는 텍스트 블록 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> TextActionName;

    /** 액션 타입/카테고리를 표시하는 텍스트 블록 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> TextActionType;

    /** 액션 설명을 표시하는 리치 텍스트 블록 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<URichTextBlock> TextActionDescription;

    /** 액션 아이콘을 표시하는 이미지 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UImage> ImageActionIcon;

    /** 쿨다운 정보를 표시하는 텍스트 블록 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> TextCooldown;

    /** 마나 비용을 표시하는 텍스트 블록 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UTextBlock> TextManaCost;

    /** 툴팁 배경 보더 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UBorder> BorderTooltipBackground;

    /** 추가 정보를 담을 수 있는 수직 박스 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UVerticalBox> VerticalBoxContent;

protected:
    /** 현재 표시 중인 액션 스펙 */
    UPROPERTY()
    FLuxActionSpecHandle ActionSpecHandle;

    /** 수식 평가기 인스턴스 */
    TUniquePtr<ExpressionEvaluator> ExpressionEval;
};
