// Fill out your copyright notice in the Description page of Project Settings.


#include "LuxCombatManager.h"
#include "LuxAssetManager.h"
#include "LuxGameData.h"
#include "LuxEffectData.h"
#include "LuxCrowdControlData.h"
#include "LuxLogChannels.h"

#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Effects/LuxEffect.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "ActionSystem/ActionSystemGlobals.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "LuxGameplayTags.h"
#include "CrowdControl/Action/LuxAction_CrowdControlBase.h"

void ULuxCombatManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// GameData에서 CrowdControlTable 로드
	ULuxAssetManager& AssetManager = ULuxAssetManager::Get();
	const ULuxGameData& GameData = ULuxGameData::Get();

	ULuxCrowdControlData* Data = AssetManager.GetAsset(GameData.CrowdControlData);
	if (!Data)
	{
		UE_LOG(LogLux, Error, TEXT("ULuxCombatManager: Failed to load CrowdControlData from GameData"));
		return;
	}

	UDataTable* CrowdControlTable = AssetManager.GetAsset(Data->CrowdControlTable);
	if (!CrowdControlTable)
	{
		UE_LOG(LogLux, Error, TEXT("ULuxCombatManager: Failed to load CrowdControlTable from CrowdControlData"));
		return;
	}

	TArray<FCrowdControlData*> AllRows;
	CrowdControlTable->GetAllRows(TEXT(""), AllRows);

	for (const FCrowdControlData* Row : AllRows)
	{
		if (Row && Row->Tag.IsValid())
		{
			CrowdControlData.Add(Row->Tag, *Row);
		}
	}

	// EffectData에서 기본 데미지 이펙트 클래스 로드
	const ULuxEffectData* EffectData = AssetManager.GetAsset(GameData.EffectData);
	if (!EffectData)
	{
		UE_LOG(LogLux, Error, TEXT("ULuxCombatManager: Failed to load EffectData from GameData"));
		return;
	}

	DefaultDamageEffectClass = AssetManager.GetSubclass(EffectData->InstantDamageEffect);
	if (!DefaultDamageEffectClass)
	{
		UE_LOG(LogLux, Error, TEXT("ULuxCombatManager: Failed to load DefaultDamageEffectClass from EffectData"));
	}
}

void ULuxCombatManager::Deinitialize()
{
	Super::Deinitialize();
	CrowdControlData.Empty();
}

const FCrowdControlData& ULuxCombatManager::GetCrowdControlData(const FGameplayTag& Tag) const
{
	return *CrowdControlData.Find(Tag);
}

TSubclassOf<ULuxEffect> ULuxCombatManager::GetDefaultDamageEffect() const
{
	return DefaultDamageEffectClass;
}


bool ULuxCombatManager::ApplyCrowdControl(UActionSystemComponent* SourceASC, UActionSystemComponent* TargetASC, const FGameplayTag Tag, float Duration, float Magnitude)
{
	if (!SourceASC || !TargetASC) return false;

	const FCrowdControlData* CCData = CrowdControlData.Find(Tag);
	if (!CCData) return false;

	if (CCData->ActionClass)
	{
		FLuxActionSpec ActionSpec(CCData->ActionClass, FGameplayTag(), 1);
		const FLuxActionSpecHandle GrantedHandle = TargetASC->GrantAction(ActionSpec);

		if (GrantedHandle.IsValid())
		{
			FLuxActionSpec* GrantedSpec = TargetASC->FindActionSpecFromHandle(GrantedHandle);
			if (GrantedSpec && GrantedSpec->Action)
			{
				ULuxAction_CrowdControlBase* CCActionInstance = Cast<ULuxAction_CrowdControlBase>(GrantedSpec->Action);
				if (CCActionInstance)
				{
					CCActionInstance->AssociatedStateTag = Tag;
				}
			}
		}
	}

	if (CCData->EffectClass)
	{
		FLuxEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.SetTargetASC(TargetASC);

		FLuxEffectSpecHandle EffectSpec = SourceASC->MakeOutgoingSpec(CCData->EffectClass, 1.0f, Context);
		if (EffectSpec.IsValid())
		{
			// 지속시간 설정
			EffectSpec.Get()->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Duration, Duration);

			// 수치 데이터 설정
			EffectSpec.Get()->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magnitude, Magnitude);

			// 효과 적용 태그 설정
			if (!EffectSpec.Get()->DynamicGrantedTags.HasTag(Tag))
			{
				EffectSpec.Get()->DynamicGrantedTags.AddTag(Tag);
			}

			FActiveLuxEffectHandle CCHandle; SourceASC->ApplyEffectSpecToTarget(EffectSpec, TargetASC, CCHandle);
		}
	}

    // 이벤트 페이로드 설정
    FContextPayload EventPayload;

    // CrowdControl 전용 구조체
	FPayload_CrowdControlEvent CCEventData;
	CCEventData.Instigator = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	CCEventData.Target = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
	CCEventData.EventLocation = TargetASC ? TargetASC->GetOwner()->GetActorLocation() : FVector::ZeroVector;
	CCEventData.EventTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    
    // CrowdControl 특화 데이터 설정
    CCEventData.Type = Tag;
    CCEventData.Duration = Duration;
    CCEventData.Magnitude = Magnitude;
    
    EventPayload.SetData(LuxPayloadKeys::CrowdControlEvent, CCEventData);

    // 피해를 받은 대상에게 "CC가 적용되었다" 이벤트 전송
    TargetASC->HandleGameplayEvent(LuxGameplayTags::Event_CrowdControl_Applied, EventPayload);
    
    // CC를 가한 가해자에게 "CC를 적용했다" 이벤트 전송 (쿨다임 감소 등 처리용)
    if (SourceASC && SourceASC != TargetASC) // 자신에게 가한 CC는 제외
    {
        SourceASC->HandleGameplayEvent(LuxGameplayTags::Event_CrowdControl_Dealt, EventPayload);
    }

	return true;

	UE_LOG(LogLux, Log, TEXT("[ULuxCombatManager] CC 이벤트 발생: %s -> %s (Tag: %s, Duration: %.1f, Magnitude: %.1f)"),
		*GetNameSafe(SourceASC->GetOwner()),
		*GetNameSafe(TargetASC->GetOwner()),
		*Tag.ToString(),
		Duration,
		Magnitude);
}


bool ULuxCombatManager::ApplyDamage(UActionSystemComponent* SourceASC, UActionSystemComponent* TargetASC, const FLuxEffectSpecHandle& DamageSpecHandle)
{
    if (!SourceASC) 
	{ 
		UE_LOG(LogLux, Error, TEXT("[%s][%s] SourceASC 가 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__));
		 return false; 
	}

    if (!TargetASC) 
	{ 
		UE_LOG(LogLux, Error, TEXT("[%s][%s] TargetASC 가 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__)); 
		return false;
	}

    if (!DamageSpecHandle.IsValid()) 
	{ 
		UE_LOG(LogLux, Error, TEXT("[%s][%s] DamageSpecHandle 가 유효하지 않습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__)); 
		return false;
	}

	// 1차 월드 레벨에서 서버 확인
    if (GetWorld()->GetNetMode() != NM_DedicatedServer && GetWorld()->GetNetMode() != NM_ListenServer) 
	{ 
		UE_LOG(LogLux, Error, TEXT("[%s][%s] 월드 레벨에서 서버가 아닙니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__)); 
		return false;
	}

	// 2차 SourceASC의 권한 확인
	if (!SourceASC->GetOwnerActor() || !SourceASC->GetOwnerActor()->HasAuthority()) 
	{ 
		UE_LOG(LogLux, Error, TEXT("[%s][%s] SourceASC에 서버 권한이 없습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__)); 
		return false;
	}

    // 적용 시도
    FActiveLuxEffectHandle AppliedHandle;
    const bool bDamageApplied = SourceASC->ApplyEffectSpecToTarget(DamageSpecHandle, TargetASC, AppliedHandle);
    if (!bDamageApplied) 
	{ 
		UE_LOG(LogLux, Error, TEXT("[%s][%s] 데미지 효과 적용에 실패했습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__)); 
		return false;
	}

    // 데미지 스펙 확보 (즉시형 판단/로그/이벤트용)
    const FLuxEffectSpec* DamageSpec = DamageSpecHandle.Get();
    if (!DamageSpec) 
	{ 
		UE_LOG(LogLux, Error, TEXT("[%s][%s] DamageSpecHandle에서 Spec을 가져올 수 없습니다."), *GetNameSafe(this), ANSI_TO_TCHAR(__FUNCTION__)); 
		return bDamageApplied;
	}

    BroadcastDamagedEvent(SourceASC, TargetASC, *DamageSpec);
	return bDamageApplied;
}

void ULuxCombatManager::BroadcastDamagedEvent(UActionSystemComponent* SourceASC, UActionSystemComponent* TargetASC, const FLuxEffectSpec& Spec)
{
    FContextPayload EventPayload;

    // Damage 전용 구조체 사용 - 명확하고 타입 안전
	FPayload_DamageEvent DamageEventData;
	DamageEventData.Instigator = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	DamageEventData.Target = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
	DamageEventData.EventLocation = TargetASC ? TargetASC->GetOwner()->GetActorLocation() : FVector::ZeroVector;
	DamageEventData.EventTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	DamageEventData.EventLevel  = Spec.Level;

    // 데미지 정보 직접 설정 - 명확하고 간단
    DamageEventData.Amount = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magnitude, false, 0.0f);
    DamageEventData.bIsCritical = Spec.DynamicGrantedTags.HasTag(LuxGameplayTags::Effect_Type_Critical);
    
    // 치명타인 경우 이벤트 태그에도 추가
    if (DamageEventData.bIsCritical)
    {
        DamageEventData.DamageTypes.AddTag(LuxGameplayTags::Effect_Type_Critical);
    }
    
    // 피해 타입들 설정
    if (Spec.DynamicGrantedTags.HasTag(LuxGameplayTags::Effect_Type_Physical))
    { 
        DamageEventData.DamageTypes.AddTag(LuxGameplayTags::Effect_Type_Physical);
    }
    if (Spec.DynamicGrantedTags.HasTag(LuxGameplayTags::Effect_Type_Magical)) 
    { 
        DamageEventData.DamageTypes.AddTag(LuxGameplayTags::Effect_Type_Magical);
    }

    // Source 액션 식별 태그 설정
    const FLuxActionSpecHandle SourceHandle = Spec.ContextHandle.GetSourceAction();
    if (SourceASC && SourceHandle.IsValid())
    {
        if (const FLuxActionSpec* SourceSpec = SourceASC->FindActionSpecFromHandle(SourceHandle))
        {
            if (SourceSpec && SourceSpec->Action)
            {
				DamageEventData.SourceActionTag = SourceSpec->Action->ActionIdentifierTag;
				DamageEventData.SourceActionTypeTag = SourceSpec->Action->ActionTags.First();
            }
        }
    }
    
    EventPayload.SetData(LuxPayloadKeys::DamageEvent, DamageEventData);
    
    // 피해를 받은 대상에게 "데미지를 받았다" 이벤트 전송
    TargetASC->HandleGameplayEvent(LuxGameplayTags::Event_Character_Damaged, EventPayload);
    
    // 피해를 가한 가해자에게 "데미지를 가했다" 이벤트 전송 (쿨다임 감소 등 처리용)
    if (SourceASC && SourceASC != TargetASC) // 자신에게 가한 데미지는 제외
    {
        SourceASC->HandleGameplayEvent(LuxGameplayTags::Event_Character_DealtDamage, EventPayload);
    }
}


void ULuxCombatManager::RecordCombatLog(const FLuxEffectSpec& Spec, float FinalDamage)
{
	FCombatLogEntry LogEntry;

	/* ==== 기본 정보 설정 ==== */
	LogEntry.Timestamp = GetWorld()->GetTimeSeconds();
	LogEntry.DamageCauser = Spec.ContextHandle.GetInstigator();
	LogEntry.DamageTarget = Spec.ContextHandle.GetTargetASC()->GetAvatarActor();
	LogEntry.DamageAmount = FinalDamage;

	/* ==== 액션 정보 설정 ==== */
	const FLuxActionSpecHandle SourceActionSpecHandle = Spec.ContextHandle.GetSourceAction();
	if (SourceActionSpecHandle.IsValid())
	{
		if (const FLuxActionSpec* SourceActionSpec = Spec.ContextHandle.GetSourceASC()->FindActionSpecFromHandle(SourceActionSpecHandle))
		{
			if (SourceActionSpec->Action)
			{
				LogEntry.SourceActionTag = SourceActionSpec->Action->ActionIdentifierTag;
			}
		}
	}

	/* ==== 피해 타입 및 상세 정보 설정 ==== */
	if (Spec.DynamicGrantedTags.HasTag(LuxGameplayTags::Effect_Type_Physical))
	{
		LogEntry.DamageTypes.AddTag(LuxGameplayTags::Effect_Type_Physical);

		// 물리 피해 상세 정보 추출
		float PhysicalBase = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Base, false, 0.0f);
		float PhysicalScale = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Scale, false, 0.0f);
		float PhysicalRaw = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Raw, false, 0.0f);
		float PhysicalFinal = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Final, false, 0.0f);

		if (PhysicalFinal > 0.0f)
		{
			LogEntry.DamageTypeDetails.Add(FDamageTypeInfo(
				LuxGameplayTags::Effect_Type_Physical,
				PhysicalFinal,
				PhysicalScale
			));
		}
	}

	if (Spec.DynamicGrantedTags.HasTag(LuxGameplayTags::Effect_Type_Magical))
	{
		LogEntry.DamageTypes.AddTag(LuxGameplayTags::Effect_Type_Magical);

		// 마법 피해 상세 정보 추출
		float MagicalBase = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Base, false, 0.0f);
		float MagicalScale = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Scale, false, 0.0f);
		float MagicalRaw = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Raw, false, 0.0f);
		float MagicalFinal = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Final, false, 0.0f);

		if (MagicalFinal > 0.0f)
		{
			LogEntry.DamageTypeDetails.Add(FDamageTypeInfo(
				LuxGameplayTags::Effect_Type_Magical,
				MagicalFinal,
				MagicalScale
			));
		}
	}

	/* ==== 치명타 여부 확인 ==== */
	LogEntry.bIsCritical = Spec.DynamicGrantedTags.HasTag(LuxGameplayTags::Effect_Type_Critical);

	/* ==== 피해 위치 정보 설정 ==== */
	if (LogEntry.DamageTarget.IsValid())
	{
		LogEntry.ImpactLocation = LogEntry.DamageTarget->GetActorLocation();
	}

	/* ==== 추가 이벤트 태그들 복사 ==== */
	LogEntry.EventTags = Spec.DynamicGrantedTags;

	/* ==== 상세한 피해 계산 결과 설정 ==== */
	// 입력 데이터
	float PhysicalBase = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Base, false, 0.0f);
	float MagicalBase = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Base, false, 0.0f);
	float PhysicalScale = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Scale, false, 0.0f);
	float MagicalScale = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Scale, false, 0.0f);

	// 중간 계산 결과
	float RawPhysicalDamage = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Raw, false, 0.0f);
	float RawMagicalDamage = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Raw, false, 0.0f);

	// 최종 결과
	float PhysicalFinal = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Final, false, 0.0f);
	float MagicalFinal = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Final, false, 0.0f);

	// 치명타 정보
	float CriticalMultiplier = Spec.GetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Critical_Multiplier, false, 1.0f);

	LogEntry.DamageCalculation = FDamageCalculationResult(
		FinalDamage,                    // TotalDamage
		PhysicalFinal,                  // PhysicalDamage
		MagicalFinal,                   // MagicalDamage
		RawPhysicalDamage,              // RawPhysicalDamage
		RawMagicalDamage,               // RawMagicalDamage
		PhysicalScale,                  // PhysicalDamageScale
		MagicalScale,                   // MagicalDamageScale
		LogEntry.bIsCritical,           // bIsCritical
		CriticalMultiplier              // CriticalMultiplier
	);

	/* ==== 로그 크기 제한 (최대 1000개 항목 유지) ==== */
	const int32 MaxLogEntries = 1000;
	if (CombatLog.Num() >= MaxLogEntries)
	{
		// 가장 오래된 항목들을 제거 (앞쪽 20% 제거)
		int32 RemoveCount = FMath::Max(1, MaxLogEntries / 5);
		CombatLog.RemoveAt(0, RemoveCount);
	}

	CombatLog.Add(LogEntry);

	/* ==== 디버그 로그 출력 ==== */
	UE_LOG(LogLuxCombat, Log, TEXT("[CombatLog] 데미지 기록: %s -> %s, 총 피해: %.1f (물리: %.1f, 마법: %.1f, 치명타: %s)"),
		*GetNameSafe(LogEntry.DamageCauser.Get()),
		*GetNameSafe(LogEntry.DamageTarget.Get()),
		FinalDamage,
		PhysicalFinal,
		MagicalFinal,
		LogEntry.bIsCritical ? TEXT("예") : TEXT("아니오"));
}

void ULuxCombatManager::ClearCombatLog(int32 MaxEntries)
{
	if (CombatLog.Num() > MaxEntries)
	{
		const int32 RemoveCount = CombatLog.Num() - MaxEntries;
		CombatLog.RemoveAt(0, RemoveCount);
		UE_LOG(LogLux, Log, TEXT("[CombatLog] %d개의 오래된 로그 항목을 제거했습니다."), RemoveCount);
	}
}

TArray<FCombatLogEntry> ULuxCombatManager::GetCombatLogInTimeRange(float StartTime, float EndTime) const
{
	TArray<FCombatLogEntry> Result;

	for (const FCombatLogEntry& Entry : CombatLog)
	{
		if (Entry.Timestamp >= StartTime && Entry.Timestamp <= EndTime)
		{
			Result.Add(Entry);
		}
	}

	return Result;
}

TArray<FCombatLogEntry> ULuxCombatManager::GetCombatLogForActor(const AActor* Actor, bool bAsDamageCauser) const
{
	TArray<FCombatLogEntry> Result;

	if (!Actor)
	{
		return Result;
	}

	for (const FCombatLogEntry& Entry : CombatLog)
	{
		if (bAsDamageCauser)
		{
			if (Entry.DamageCauser.Get() == Actor)
			{
				Result.Add(Entry);
			}
		}
		else
		{
			if (Entry.DamageTarget.Get() == Actor)
			{
				Result.Add(Entry);
			}
		}
	}

	return Result;
}