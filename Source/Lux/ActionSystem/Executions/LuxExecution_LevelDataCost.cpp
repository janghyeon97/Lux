// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Executions/LuxExecution_LevelDataCost.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Attributes/CombatSet.h"
#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"


void ULuxExecution_LevelDataCost::Execute_Implementation(FLuxEffectSpec& Spec) const
{
	const FLuxEffectContextHandle& ContextHandle = Spec.ContextHandle;
	if (!ContextHandle.IsValid()) return;

	const FLuxActionSpecHandle SourceActionSpecHandle = ContextHandle.GetSourceAction();
	if (!SourceActionSpecHandle.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("FromLevelData Cost Execution failed: Source Action Spec Handle is invalid."));
		return;
	}

	UActionSystemComponent* SourceASC = ContextHandle.GetSourceASC();
	if (!SourceASC)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("FromLevelData Cost Execution failed: Source ASC is invalid."));
		return;
	}

	const FLuxActionSpec* SourceActionSpec = SourceASC->FindActionSpecFromHandle(SourceActionSpecHandle);
	if (!SourceActionSpec || !SourceActionSpec->Action || !SourceActionSpec->Action->LevelDataTable)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("FromLevelData Cost Execution failed: Source Action or LevelDataTable is invalid."));
		return;
	}

	// 액션 레벨에 맞는 데이터를 가져옵니다.
	const FName RowName = FName(*FString::FromInt(Spec.Level));
	const FLuxActionLevelData* LevelData = SourceActionSpec->Action->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("CostExecution"));
	if (!LevelData)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("FromLevelData Cooldown Execution failed: Could not find LevelData for level %d."), (int32)Spec.Level);
		return;
	}

	const FActionLevelDataBase* ActionLevelData = LevelData->ActionSpecificData.GetPtr<FActionLevelDataBase>();
	if (!ActionLevelData) return;

	// 데이터 테이블의 Cost 값을 이펙트의 비용으로 설정합니다.
	Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Cost, -ActionLevelData->Cost);
}
