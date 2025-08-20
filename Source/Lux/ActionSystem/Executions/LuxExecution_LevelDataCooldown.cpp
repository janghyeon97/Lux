// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Executions/LuxExecution_LevelDataCooldown.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxActionLevelData.h"
#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"



void ULuxExecution_LevelDataCooldown::Execute_Implementation(FLuxEffectSpec& Spec) const
{
	const FLuxEffectContextHandle& ContextHandle = Spec.ContextHandle;
	if (!ContextHandle.IsValid()) return;

	const FLuxActionSpecHandle ActionSpecHandle = ContextHandle.GetSourceAction();
	if (!ActionSpecHandle.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 액션 스펙 핸들이 유효하지 않습니다."));
		return;
	}

	UActionSystemComponent* SourceASC = ContextHandle.GetSourceASC();
	if (!SourceASC)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 소스 액션 시스템 컴포넌트가 유효하지 않습니다."));
		return;
	}

	const FLuxActionSpec* ActionSpec = SourceASC->FindActionSpecFromHandle(ActionSpecHandle);
	if (!ActionSpec)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 핸들로부터 액션 스펙을 찾을 수 없습니다."));
		return;
	}

	ULuxAction* SourceAction = ActionSpec->Action;
	if (!SourceAction || !SourceAction->LevelDataTable)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] 소스 액션이나 레벨 데이터 테이블이 유효하지 않습니다."), *GetNameSafe(SourceAction));
		return;
	}

	// 액션 레벨에 맞는 데이터를 가져옵니다.
	const FName RowName = FName(*FString::FromInt(Spec.Level));
	const FLuxActionLevelData* LevelData = SourceAction->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("CooldownExecution"));
	if (!LevelData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 레벨 %d에 대한 레벨 데이터를 찾을 수 없습니다."), (int32)Spec.Level);
		return;
	}

	const FActionLevelDataBase* ActionLevelData = LevelData->ActionSpecificData.GetPtr<FActionLevelDataBase>();
	if (!ActionLevelData) 
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("레벨 데이터 기반 쿨다운 실행 실패: 레벨 데이터에서 FActionLevelDataBase 타입의 데이터를 찾을 수 없습니다."));
		return;
	}

	// 데이터 테이블의 쿨다운 값을 이펙트의 지속시간으로 설정합니다.
	const float CooldownDuration = FMath::Max(ActionLevelData->Cooldown, 0.1f);
	Spec.SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Duration, CooldownDuration);
	UE_LOG(LogLuxActionSystem, Log, TEXT("레벨 데이터 기반 쿨다운 실행: 쿨다운 값 %f"), CooldownDuration);

	const FGameplayTag CooldownTag = ActionSpec->GetCooldownTag();
	if (CooldownTag.IsValid())
	{
		Spec.DynamicGrantedTags.AddTag(CooldownTag);
		UE_LOG(LogLuxActionSystem, Log, TEXT("레벨 데이터 기반 쿨다운 실행: 쿨다운 태그 %s"), *CooldownTag.ToString());
	}
}
