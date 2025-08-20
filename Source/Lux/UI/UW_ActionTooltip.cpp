// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UW_ActionTooltip.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Effects/LuxEffect.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"

#include "System/StatMappingData.h"

#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "TimerManager.h"
#include "LuxLogChannels.h"


UUW_ActionTooltip::UUW_ActionTooltip(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UUW_ActionTooltip::NativeConstruct()
{
	Super::NativeConstruct();

	// ExpressionEvaluator 인스턴스 생성
	if (!ExpressionEval.IsValid())
	{
		ExpressionEval = MakeUnique<ExpressionEvaluator>();
	}
}

void UUW_ActionTooltip::NativeDestruct()
{
	Super::NativeDestruct();

}

void UUW_ActionTooltip::InitializeTooltip(UActionSystemComponent* InASC, const FLuxActionSpec& ActionSpec)
{
	if (!ActionSpec.Action)
	{
		UE_LOG(LogLux, Error, TEXT("[%s] ActionSpec.Action이 유효하지 않습니다."), *GetNameSafe(this));
		return;
	}

	if (!InASC)
	{
		UE_LOG(LogLux, Error, TEXT("[%s] ActionSystemComponent가 유효하지 않습니다."), *GetNameSafe(this));
		return;
	}

	ActionSpecHandle = ActionSpec.Handle;
	ASC = InASC;

	// 초기 툴팁 데이터 업데이트
	UpdateTooltipText(ActionSpec);
}


void UUW_ActionTooltip::UpdateTooltipData()
{
	if (!ASC.IsValid() || !ActionSpecHandle.IsValid())
	{
		return;
	}

	// ActionSpecHandle을 사용하여 현재 ActionSpec 가져오기
	const FLuxActionSpec* CurrentActionSpec = ASC->FindActionSpecFromHandle(ActionSpecHandle);
	if (!CurrentActionSpec || !CurrentActionSpec->Action)
	{
		return;
	}

	// LuxAction에서 직접 데이터를 가져와서 텍스트만 업데이트 (아이콘은 ActionIcon에서 전달)
	UpdateTooltipText(*CurrentActionSpec);
}


/**
 * 액션 설명 텍스트의 동적 수식을 평가하여 실제 수치로 치환하고 RichText 스타일을 적용합니다.
 *
 * 이 함수는 3단계로 텍스트를 처리합니다:
 * 1. ReplaceStatReferences: {StatName} → 캐릭터 스탯 값
 * 2. ReplaceActionDataReferences: @ActionData@ → 액션 레벨 데이터 값
 * 3. EvaluateExpressions: [Expression] → 계산된 수식 결과
 *
 * 지원하는 문법:
 * - {StatName}: 캐릭터 스탯 참조 (예: {AD}, {AP}, {HP})
 * - @ActionData@: 액션 레벨 데이터 참조 (예: @Cooldown@, @Cost@)
 * - [Expression]: 수식 계산 (예: [@AttackDamageBase@ + {AD} * @AbilityPowerScale@])
 * - <style>content</style>: RichText 스타일 적용 (예: <phy>, <mag>, <heal>)
 *
 * 예시: "<phy>[@AttackDamageBase@ + {AD} * @AbilityPowerScale@]</phy> 물리 피해"
 *       → "<phy>135.0</phy> 물리 피해"
 *
 * @param DescriptionText 평가할 원본 설명 텍스트
 * @return 모든 동적 요소가 실제 값으로 치환된 최종 텍스트
 */
FString UUW_ActionTooltip::EvaluateDescriptionText(const FString& DescriptionText) const
{
	if (DescriptionText.IsEmpty())
	{
		return DescriptionText;
	}

	// ExpressionEval이 없으면 지연 생성
	if (!ExpressionEval.IsValid())
	{
		const_cast<UUW_ActionTooltip*>(this)->ExpressionEval = MakeUnique<ExpressionEvaluator>();
		UE_LOG(LogLux, Log, TEXT("[%s] ExpressionEvaluator 지연 생성 완료"), *GetNameSafe(this));
	}

	FString ProcessedText = DescriptionText;

	// 1단계: 캐릭터 스탯 참조 치환
	ProcessedText = ReplaceStatReferences(ProcessedText);

	// 2단계: 액션 레벨 데이터 참조 치환
	ProcessedText = ReplaceActionDataReferences(ProcessedText);

	// 3단계: 수식 계산 및 치환
	ProcessedText = EvaluateExpressions(ProcessedText);

	return ProcessedText;
}

/**
 * 텍스트에서 {StatName} 패턴을 찾아 캐릭터 스탯 값으로 치환합니다.
 *
 * {StatName} 형태의 패턴을 찾아서 실제 캐릭터의 현재 스탯 값으로 교체합니다.
 * 스탯 값은 <stat> 태그로 래핑되어 RichText 스타일이 적용됩니다.
 *
 * 예시: {AD} → <stat>85.0</stat>, {HP} → <stat>1200.0</stat>
 *
 * @param Text 스탯 참조를 포함한 원본 텍스트
 * @return 스탯 참조가 실제 값으로 치환된 텍스트
 */
FString UUW_ActionTooltip::ReplaceStatReferences(const FString& Text) const
{
	FString ProcessedText = Text;
	FRegexPattern StatPattern(TEXT("\\{([A-Za-z]+)\\}"));
	FRegexMatcher StatMatcher(StatPattern, ProcessedText);

	while (StatMatcher.FindNext())
	{
		FString StatName = StatMatcher.GetCaptureGroup(1);
		float StatValue = GetPlayerStatValue(StatName);

		// 스탯 참조를 실제 값으로 교체 (RichText 스타일 적용)
		FString StyledStatValue = FString::Printf(TEXT("%.1f"), StatValue);
		ProcessedText = ProcessedText.Replace(*StatMatcher.GetCaptureGroup(0), *StyledStatValue);
	}

	return ProcessedText;
}

/**
 * 텍스트에서 @ActionData@ 패턴을 찾아 액션 레벨 데이터 값으로 치환합니다.
 *
 * @DataName@ 형태의 패턴을 찾아서 현재 액션의 레벨별 데이터 값으로 교체합니다.
 * 액션 레벨 데이터는 FInstancedStruct에서 리플렉션을 통해 추출됩니다.
 *
 * 예시: @AttackDamageBase@ → 87.5, @Cooldown@ → 10.0
 *
 * @param Text 액션 데이터 참조를 포함한 원본 텍스트
 * @return 액션 데이터 참조가 실제 값으로 치환된 텍스트
 */
FString UUW_ActionTooltip::ReplaceActionDataReferences(const FString& Text) const
{
	FString ProcessedText = Text;
	FRegexPattern ActionDataPattern(TEXT("@([A-Za-z][A-Za-z0-9_]*)@"));
	FRegexMatcher ActionDataMatcher(ActionDataPattern, ProcessedText);

	while (ActionDataMatcher.FindNext())
	{
		FString DataName = ActionDataMatcher.GetCaptureGroup(1);
		float DataValue = GetActionLevelData(DataName);

		// 액션 데이터를 실제 값으로 교체
		FString DataValueText = FString::Printf(TEXT("%.1f"), DataValue);
		ProcessedText = ProcessedText.Replace(*ActionDataMatcher.GetCaptureGroup(0), *DataValueText);
	}

	return ProcessedText;
}

/**
 * 텍스트에서 [Expression] 패턴을 찾아 수식을 계산한 결과로 치환합니다.
 *
 * [수식] 형태의 패턴을 찾아서 ExpressionEvaluator를 사용해 계산한 결과로 교체합니다.
 * 수식 내부에 포함된 스탯 참조나 액션 데이터 참조도 먼저 실제 값으로 치환한 후 계산합니다.
 *
 * 예시: [50.0 + 85.0 * 1.2] → 152.0
 *
 * @param Text 수식을 포함한 원본 텍스트
 * @return 수식이 계산 결과로 치환된 텍스트
 */
FString UUW_ActionTooltip::EvaluateExpressions(const FString& Text) const
{
	if (!ExpressionEval.IsValid())
	{
		return Text;
	}

	FString ProcessedText = Text;
	FRegexPattern ExpressionPattern(TEXT("\\[([^\\]]+)\\]"));
	FRegexMatcher ExpressionMatcher(ExpressionPattern, ProcessedText);

	// 수식 내부에서 사용할 패턴들
	FRegexPattern StatPattern(TEXT("\\{([A-Za-z]+)\\}"));
	FRegexPattern ActionDataPattern(TEXT("@([A-Za-z][A-Za-z0-9_]*)@"));

	while (ExpressionMatcher.FindNext())
	{
		FString Expression = ExpressionMatcher.GetCaptureGroup(1);

		// 수식 내부의 스탯 참조를 실제 값으로 치환
		FRegexMatcher InnerStatMatcher(StatPattern, Expression);
		while (InnerStatMatcher.FindNext())
		{
			FString StatName = InnerStatMatcher.GetCaptureGroup(1);
			float StatValue = GetPlayerStatValue(StatName);
			FString StatValueText = FString::Printf(TEXT("%.1f"), StatValue);
			Expression = Expression.Replace(*InnerStatMatcher.GetCaptureGroup(0), *StatValueText);
		}

		// 수식 내부의 액션 레벨 데이터를 실제 값으로 치환
		FRegexMatcher InnerActionDataMatcher(ActionDataPattern, Expression);
		while (InnerActionDataMatcher.FindNext())
		{
			FString DataName = InnerActionDataMatcher.GetCaptureGroup(1);
			float DataValue = GetActionLevelData(DataName);
			FString DataValueText = FString::Printf(TEXT("%.1f"), DataValue);
			Expression = Expression.Replace(*InnerActionDataMatcher.GetCaptureGroup(0), *DataValueText);
		}

		// ExpressionEvaluator로 수식 계산 실행
		double Result = 0.0;
		if (ExpressionEval->Evaluate(TCHAR_TO_UTF8(*Expression), Result))
		{
			// 계산 결과로 단순히 수치만 치환 (외부 스타일 태그는 그대로 유지)
			FString ResultText = FString::Printf(TEXT("%.1f"), Result);
			ProcessedText = ProcessedText.Replace(*ExpressionMatcher.GetCaptureGroup(0), *ResultText);
		}
		else
		{
			// 평가 실패 시 원본 텍스트 유지하고 경고 로그 출력
			UE_LOG(LogLux, Warning, TEXT("[%s] 수식 평가 실패: %s"), *GetNameSafe(this), *Expression);
		}
	}

	return ProcessedText;
}

float UUW_ActionTooltip::GetPlayerStatValue(const FString& StatName) const
{
	if (!ASC.IsValid())
	{
		return 0.0f;
	}

	// 스탯 맵핑 시스템을 사용하여 값 가져오기
	const UStatMappingData& StatMapping = UStatMappingData::Get();

	// 디버그: 첫 번째 마우스 오버 문제 해결을 위한 로그
	if (StatMapping.CachedStatMappings.Num() == 0)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] StatMapping 캐시가 비어있습니다. StatName: %s"), *GetNameSafe(this), *StatName);
	}

	return StatMapping.GetStatValue(StatName, ASC.Get());
}

float UUW_ActionTooltip::GetActionLevelData(const FString& DataName) const
{
	if (!ASC.IsValid() || !ActionSpecHandle.IsValid())
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] ASC 또는 ActionSpecHandle이 유효하지 않습니다. 데이터: %s"), *GetNameSafe(this), *DataName);
		return 0.0f;
	}

	// ActionSpecHandle을 사용하여 현재 ActionSpec 가져오기
	const FLuxActionSpec* CurrentActionSpec = ASC->FindActionSpecFromHandle(ActionSpecHandle);
	if (!CurrentActionSpec || !CurrentActionSpec->Action)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] 액션 스펙이 유효하지 않습니다. 데이터: %s"), *GetNameSafe(this), *DataName);
		return 0.0f;
	}

	ULuxAction* Action = CurrentActionSpec->Action;

	// 액션의 현재 레벨 가져오기
	int32 ActionLevel = CurrentActionSpec->Level;
	if (ActionLevel <= 0)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] 액션 레벨이 유효하지 않습니다. Level: %d, 데이터: %s"), *GetNameSafe(this), ActionLevel, *DataName);
		return 0.0f;
	}

	// 액션의 LevelData 테이블에서 해당 레벨의 데이터 가져오기
	if (!Action->LevelDataTable)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] 액션에 LevelData 테이블이 설정되지 않았습니다. 액션: %s, 데이터: %s"),
			*GetNameSafe(this), *Action->GetName(), *DataName);
		return 0.0f;
	}

	// 레벨에 해당하는 행 이름 생성 (예: "1", "2" 등)
	FName LevelRowName = FName(*FString::Printf(TEXT("%d"), ActionLevel));

	// 데이터 테이블에서 해당 레벨의 행 찾기
	FLuxActionLevelData* LevelDataRow = Action->LevelDataTable->FindRow<FLuxActionLevelData>(LevelRowName, TEXT("GetActionLevelData"));
	if (!LevelDataRow)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] 레벨 데이터를 찾을 수 없습니다. 액션: %s, 레벨: %d, 행이름: %s"),
			*GetNameSafe(this), *Action->GetName(), ActionLevel, *LevelRowName.ToString());
		return 0.0f;
	}

	// FInstancedStruct에서 DataName에 해당하는 필드 값 추출
	return ExtractValueFromInstancedStruct(LevelDataRow->ActionSpecificData, DataName);
}

float UUW_ActionTooltip::ExtractValueFromInstancedStruct(const FInstancedStruct& InstancedStruct, const FString& FieldName) const
{
	if (!InstancedStruct.IsValid())
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] InstancedStruct가 유효하지 않습니다. 필드: %s"), *GetNameSafe(this), *FieldName);
		return 0.0f;
	}

	// Struct의 타입 정보 가져오기
	const UScriptStruct* ScriptStruct = InstancedStruct.GetScriptStruct();
	if (!ScriptStruct)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] ScriptStruct를 가져올 수 없습니다. 필드: %s"), *GetNameSafe(this), *FieldName);
		return 0.0f;
	}

	// Struct 데이터의 포인터 가져오기
	const void* StructPtr = InstancedStruct.GetMemory();
	if (!StructPtr)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] Struct 메모리 포인터를 가져올 수 없습니다. 필드: %s"), *GetNameSafe(this), *FieldName);
		return 0.0f;
	}

	// 필드 이름으로 프로퍼티 찾기
	const FProperty* Property = ScriptStruct->FindPropertyByName(*FieldName);
	if (!Property)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] 필드를 찾을 수 없습니다. Struct: %s, 필드: %s"),
			*GetNameSafe(this), *ScriptStruct->GetName(), *FieldName);
		return 0.0f;
	}

	if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		return FloatProp->GetPropertyValue_InContainer(StructPtr);
	}
	else if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		return static_cast<float>(DoubleProp->GetPropertyValue_InContainer(StructPtr));
	}
	else if (const FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		return static_cast<float>(IntProp->GetPropertyValue_InContainer(StructPtr));
	}
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		return static_cast<float>(ByteProp->GetPropertyValue_InContainer(StructPtr));
	}
	else if (const FUInt32Property* UInt32Prop = CastField<FUInt32Property>(Property))
	{
		return static_cast<float>(UInt32Prop->GetPropertyValue_InContainer(StructPtr));
	}
	else if (const FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
	{
		return static_cast<float>(Int64Prop->GetPropertyValue_InContainer(StructPtr));
	}
	else
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] 지원하지 않는 프로퍼티 타입입니다. 필드: %s, 타입: %s"),
			*GetNameSafe(this), *FieldName, *Property->GetClass()->GetName());
		return 0.0f;
	}
}

void UUW_ActionTooltip::UpdateTooltipText(const FLuxActionSpec& ActionSpec)
{
	if (!ASC.IsValid() || !ActionSpec.Action)
	{
		UE_LOG(LogLux, Error, TEXT("[%s] ASC 또는 ActionSpec.Action이 유효하지 않습니다."), *GetNameSafe(this));
		return;
	}

	ULuxAction* Action = ActionSpec.Action;

	// 액션 이름 설정
	if (TextActionName)
	{
		FText ActionName = Action->DisplayName.IsEmpty() ?
			FText::FromString(Action->GetClass()->GetName()) : Action->DisplayName;
		TextActionName->SetText(ActionName);
	}

	// 액션 타입/카테고리 설정 (액션 태그 기반)
	if (TextActionType)
	{
		FString TypeText = TEXT("스킬");
		if (Action->ActionTags.Num() > 0)
		{
			TypeText = Action->ActionTags.First().ToString();
		}
		TextActionType->SetText(FText::FromString(TypeText));
	}

	// 액션 설명 설정 (수식 평가 포함)
	if (TextActionDescription)
	{
		FString DescriptionText = Action->Description.ToString();
		FString EvaluatedDescription = EvaluateDescriptionText(DescriptionText);
		TextActionDescription->SetText(FText::FromString(EvaluatedDescription));
	}

	// 쿨다운 정보 설정 (레벨 데이터에서 가져오기)
	if (TextCooldown)
	{
		float CooldownValue = GetActionLevelData(TEXT("Cooldown"));
		if (CooldownValue > 0.0f)
		{
			TextCooldown->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), CooldownValue)));
		}
		else
		{
			TextCooldown->SetText(FText::FromString(TEXT("0")));
		}
	}

	// 비용 정보 설정 (레벨 데이터에서 가져오기)
	if (TextManaCost)
	{
		float CostValue = GetActionLevelData(TEXT("Cost"));
		if (CostValue > 0.0f)
		{
			TextManaCost->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), CostValue)));
		}
		else
		{
			TextManaCost->SetText(FText::FromString(TEXT("0")));
		}
	}
}

void UUW_ActionTooltip::UpdateTooltipIcon(UTexture2D* IconTexture)
{
	if (ImageActionIcon && IconTexture)
	{
		ImageActionIcon->SetBrushFromTexture(IconTexture);
	}
}