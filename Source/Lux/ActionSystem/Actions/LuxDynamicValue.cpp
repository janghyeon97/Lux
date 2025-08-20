#include "ActionSystem/Actions/LuxDynamicValue.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxActionLevelData.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Actions/LuxPayload.h"


/**
 * FDynamicVector의 최종 FVector 값을 런타임에 결정하여 반환합니다.
 */
FVector FDynamicVector::GetValue(ULuxAction* InAction) const
{
	// 액션이 유효하지 않으면 기본값을 반환합니다.
	if (!InAction)
	{
		return StaticValue;
	}

	/** =============== 'FromLevelData'인 경우 =============== */
	if (Source == EPhaseParameterSource::FromLevelData)
	{
		// ASC 를 통해 FActiveLuxAction 검색합니다.
		const FActiveLuxAction* ActiveAction = InAction->GetActionSystemComponent()->FindActiveAction(InAction->GetActiveHandle());
		if (!InAction->LevelDataTable || !ActiveAction || DataKey.IsNone())
		{
			return StaticValue;
		}

		// LevelDataTable 에서 액션 레벨에 해당하는 FLuxActionLevelData 을 검색합니다.
		const FName RowName = FName(*FString::FromInt(ActiveAction->Spec.Level));
		const FLuxActionLevelData* LevelData = InAction->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT(""));
		if (!LevelData)
		{
			return StaticValue;
		}

		// 리플렉션을 사용하여 DataKey와 일치하는 이름의 프로퍼티를 찾습니다.
		const UScriptStruct* StructType = LevelData->ActionSpecificData.GetScriptStruct();
		const void* StructMemory = LevelData->ActionSpecificData.GetMemory();
		FProperty* FoundProperty = StructType ? StructType->FindPropertyByName(DataKey) : nullptr;
		if (!FoundProperty)
		{
			return StaticValue;
		}

		// 찾은 프로퍼티가 FVector 타입이라면 값을 반환합니다.
		if (auto* StructProp = CastField<FStructProperty>(FoundProperty))
		{
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				return *StructProp->ContainerPtrToValuePtr<FVector>(StructMemory);
			}
		}
	}

	/** =============== 'FromActionPayload'인 경우 =============== */
	else if (Source == EPhaseParameterSource::FromActionPayload)
	{
		// DataKey와 일치하는 FPayload_Vector 데이터를 찾아 값을 반환합니다.
		if (InAction->ActionPayload.IsValid() && !DataKey.IsNone())
		{
			if (const FPayload_Vector* PayloadData = InAction->ActionPayload->GetData<FPayload_Vector>(DataKey))
			{
				return PayloadData->Value;
			}
		}
	}

	// 모든 조건에 해당하지 않거나 'Static'인 경우 기본값을 반환합니다.
	return StaticValue;
}


/**
 * FDynamicRotator의 최종 FRotator 값을 런타임에 결정하여 반환합니다.
 */
FRotator FDynamicRotator::GetValue(ULuxAction* InAction) const
{
	// 액션이 유효하지 않으면 기본값을 반환합니다.
	if (!InAction)
	{
		return StaticValue;
	}

	/** =============== 'FromLevelData'인 경우 =============== */
	if (Source == EPhaseParameterSource::FromLevelData)
	{
		// ASC 를 통해 FActiveLuxAction 검색합니다.
		const FActiveLuxAction* ActiveAction = InAction->GetActionSystemComponent()->FindActiveAction(InAction->GetActiveHandle());
		if (!InAction->LevelDataTable || !ActiveAction || DataKey.IsNone())
		{
			return StaticValue;
		}

		// LevelDataTable 에서 액션 레벨에 해당하는 FLuxActionLevelData 을 검색합니다.
		const FName RowName = FName(*FString::FromInt(ActiveAction->Spec.Level));
		const FLuxActionLevelData* LevelData = InAction->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT(""));
		if (!LevelData)
		{
			return StaticValue;
		}

		// 리플렉션을 사용하여 DataKey와 일치하는 이름의 프로퍼티를 찾습니다.
		const UScriptStruct* StructType = LevelData->ActionSpecificData.GetScriptStruct();
		const void* StructMemory = LevelData->ActionSpecificData.GetMemory();
		FProperty* FoundProperty = StructType ? StructType->FindPropertyByName(DataKey) : nullptr;
		if (!FoundProperty)
		{
			return StaticValue; 
		}

		// 찾은 프로퍼티가 FRotator 타입이라면 값을 반환합니다.
		if (auto* StructProp = CastField<FStructProperty>(FoundProperty))
		{
			if (StructProp->Struct == TBaseStructure<FRotator>::Get())
			{
				return *StructProp->ContainerPtrToValuePtr<FRotator>(StructMemory);
			}
		}
	}

	/** =============== 'FromActionPayload'인 경우 =============== */
	else if (Source == EPhaseParameterSource::FromActionPayload)
	{
		// DataKey와 일치하는 FPayload_Rotator 데이터를 찾아 값을 반환합니다.
		if (InAction->ActionPayload.IsValid() && !DataKey.IsNone())
		{
			if (const FPayload_Rotator* PayloadData = InAction->ActionPayload->GetData<FPayload_Rotator>(DataKey))
			{
				return PayloadData->Value;
			}
		}
	}

	// 모든 조건에 해당하지 않거나 'Static'인 경우 기본값을 반환합니다.
	return StaticValue;
}

/**
 * FDynamicFloat의 최종 float 값을 런타임에 결정하여 반환합니다.
 */
float FDynamicFloat::GetValue(ULuxAction* InAction) const
{
	// 액션이 유효하지 않으면 기본값을 반환합니다.
	if (!InAction)
	{
		return StaticValue;
	}

	/** =============== 'FromLevelData'인 경우 =============== */
	if (Source == EPhaseParameterSource::FromLevelData)
	{
		// ASC 를 통해 FActiveLuxAction 검색합니다.
		const FActiveLuxAction* ActiveAction = InAction->GetActionSystemComponent()->FindActiveAction(InAction->GetActiveHandle());
		if (!InAction->LevelDataTable || !ActiveAction || DataKey.IsNone())
		{
			return StaticValue;
		}

		// LevelDataTable 에서 액션 레벨에 해당하는 FLuxActionLevelData 을 검색합니다.
		const FName RowName = FName(*FString::FromInt(ActiveAction->Spec.Level));
		const FLuxActionLevelData* LevelData = InAction->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT(""));
		if (!LevelData)
		{
			return StaticValue;
		}

		// 리플렉션을 사용하여 DataKey와 일치하는 이름의 프로퍼티를 찾습니다.
		const UScriptStruct* StructType = LevelData->ActionSpecificData.GetScriptStruct();
		const void* StructMemory = LevelData->ActionSpecificData.GetMemory();
		FProperty* FoundProperty = StructType ? StructType->FindPropertyByName(DataKey) : nullptr;
		if (!FoundProperty)
		{
			return StaticValue;
		}

		// 찾은 프로퍼티가 float 타입이라면 값을 반환합니다.
		if (auto* FloatProp = CastField<FFloatProperty>(FoundProperty))
		{
			return FloatProp->GetPropertyValue_InContainer(StructMemory);
		}
	}

	/** =============== 'FromActionPayload'인 경우 =============== */
	else if (Source == EPhaseParameterSource::FromActionPayload)
	{
		if (InAction->ActionPayload.IsValid() && !DataKey.IsNone())
		{
			// DataKey와 일치하는 FPayload_Float 데이터를 찾아 값을 반환합니다.
			if (const FPayload_Float* PayloadData = InAction->ActionPayload->GetData<FPayload_Float>(DataKey))
			{
				return PayloadData->Value;
			}
		}
	}

	// 모든 조건에 해당하지 않거나 'Static'인 경우 기본값을 반환합니다.
	return StaticValue;
}