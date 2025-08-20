// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LuxGameplayTags.h"
#include "LuxCombatManager.generated.h"

class ULuxAction;
class ULuxEffect;
class UActionSystemComponent;
class ULuxAction_CrowdControlBase;

struct FLuxEffectSpec;
struct FLuxEffectSpecHandle;

/* ==== 피해 계산 결과 정보 ==== */
USTRUCT(BlueprintType)
struct FDamageCalculationResult
{
    GENERATED_BODY()

    /* 총 피해량 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float TotalDamage = 0.0f;

    /* 물리 피해량 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float PhysicalDamage = 0.0f;

    /* 마법 피해량 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float MagicalDamage = 0.0f;

    /* 원시 물리 피해량 (방어력 적용 전) */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float RawPhysicalDamage = 0.0f;

    /* 원시 마법 피해량 (마법저항 적용 전) */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float RawMagicalDamage = 0.0f;

    /* 물리 피해 계수 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float PhysicalDamageScale = 0.0f;

    /* 마법 피해 계수 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float MagicalDamageScale = 0.0f;

    /* 치명타 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    bool bIsCritical = false;

    /* 치명타 배율 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageCalculation")
    float CriticalMultiplier = 1.0f;

    FDamageCalculationResult() = default;
    
    FDamageCalculationResult(float InTotalDamage, float InPhysicalDamage, float InMagicalDamage, 
                            float InRawPhysicalDamage, float InRawMagicalDamage,
                            float InPhysicalScale, float InMagicalScale, bool InIsCritical = false, float InCriticalMultiplier = 1.0f)
        : TotalDamage(InTotalDamage)
        , PhysicalDamage(InPhysicalDamage)
        , MagicalDamage(InMagicalDamage)
        , RawPhysicalDamage(InRawPhysicalDamage)
        , RawMagicalDamage(InRawMagicalDamage)
        , PhysicalDamageScale(InPhysicalScale)
        , MagicalDamageScale(InMagicalScale)
        , bIsCritical(InIsCritical)
        , CriticalMultiplier(InCriticalMultiplier)
    {}
};

/* ==== 피해 타입별 세부 정보 ==== */
USTRUCT(BlueprintType)
struct FDamageTypeInfo
{
    GENERATED_BODY()

    /* 피해 타입 태그 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageTypeInfo")
    FGameplayTag DamageType;

    /* 해당 타입의 피해량 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageTypeInfo")
    float DamageAmount = 0.0f;

    /* 해당 타입의 피해 계수 */
    UPROPERTY(BlueprintReadOnly, Category = "DamageTypeInfo")
    float DamageScale = 1.0f;

    FDamageTypeInfo() = default;
    FDamageTypeInfo(const FGameplayTag& InDamageType, float InDamageAmount, float InDamageScale = 1.0f)
        : DamageType(InDamageType), DamageAmount(InDamageAmount), DamageScale(InDamageScale)
    {}
};

USTRUCT(BlueprintType)
struct FCombatLogEntry
{
    GENERATED_BODY()

    /* 피해를 입힌 액터 (누가) */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    TWeakObjectPtr<AActor> DamageCauser;

    /* 피해를 받은 액터 (대상) */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    TWeakObjectPtr<AActor> DamageTarget;

    /* 사용된 액션의 태그 (무엇으로) */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    FGameplayTag SourceActionTag;

    /* 이벤트 발생 시간 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    float Timestamp = 0.0f;

    /* 총 피해량 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    float DamageAmount = 0.0f;

    /* 피해 타입들 (물리/마법 등) - 여러 타입이 동시에 있을 수 있음 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    FGameplayTagContainer DamageTypes;

    /* 피해 타입별 세부 정보 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    TArray<FDamageTypeInfo> DamageTypeDetails;

    /* 치명타 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    bool bIsCritical = false;

    /* 피해가 발생한 위치 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    FVector ImpactLocation = FVector::ZeroVector;

    /* 피해가 발생한 표면의 법선 벡터 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    FVector ImpactNormal = FVector::ZeroVector;

    /* 추가 이벤트 태그들 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    FGameplayTagContainer EventTags;

    /* 상세한 피해 계산 결과 */
    UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
    FDamageCalculationResult DamageCalculation;

    /* 피해 타입이 물리인지 확인하는 헬퍼 함수 */
    bool IsPhysicalDamage() const
    {
        return DamageTypes.HasTag(LuxGameplayTags::Effect_Type_Physical);
    }

    /* 피해 타입이 마법인지 확인하는 헬퍼 함수 */
    bool IsMagicalDamage() const
    {
        return DamageTypes.HasTag(LuxGameplayTags::Effect_Type_Magical);
    }

    /* 피해 타입이 혼합인지 확인하는 헬퍼 함수 */
    bool IsMixedDamage() const
    {
        return DamageTypes.Num() > 1;
    }

    /* 주요 피해 타입을 반환하는 헬퍼 함수 (UI 표시용) */
    FGameplayTag GetPrimaryDamageType() const
    {
        if (DamageTypes.HasTag(LuxGameplayTags::Effect_Type_Physical))
        {
            return LuxGameplayTags::Effect_Type_Physical;
        }
        else if (DamageTypes.HasTag(LuxGameplayTags::Effect_Type_Magical))
        {
            return LuxGameplayTags::Effect_Type_Magical;
        }
        return FGameplayTag();
    }

    /* 특정 피해 타입의 세부 정보를 가져오는 헬퍼 함수 */
    FDamageTypeInfo GetDamageTypeInfo(const FGameplayTag& DamageType) const
    {
        for (const FDamageTypeInfo& Info : DamageTypeDetails)
        {
            if (Info.DamageType == DamageType)
            {
                return Info;
            }
        }
        return FDamageTypeInfo();
    }

    /* 특정 피해 타입의 피해량을 가져오는 헬퍼 함수 */
    float GetDamageAmountForType(const FGameplayTag& DamageType) const
    {
        return GetDamageTypeInfo(DamageType).DamageAmount;
    }
};


/* ==== CC 데이터 구조체 ==== */
USTRUCT(BlueprintType)
struct FCrowdControlData : public FTableRowBase
{
    GENERATED_BODY()

    /* CC 기의 태그 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl", meta = (Categories = "CrowdControl"))
    FGameplayTag Tag;

    /* CC 기의 행동을 담당하는 액션 클래스 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl")
    TSubclassOf<ULuxAction_CrowdControlBase> ActionClass;

    /* CC 기의 상태 변화(이동속도 감소 등)를 담당하는 LuxEffect클래스 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl")
    TSubclassOf<ULuxEffect> EffectClass;

    FCrowdControlData()
    {

    }
};


UCLASS()
class LUX_API ULuxCombatManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
    // ~ UWorldSubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // ~ End of UWorldSubsystem interface

     /** 데이터 테이블에서 Tag에 해당하는 CC/전투 데이터를 찾아 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "CombatManager")
    const FCrowdControlData& GetCrowdControlData(const FGameplayTag& Tag) const;

    /** 시스템에서 사용할 기본 데미지 이펙트 클래스를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "CombatManager")
    TSubclassOf<ULuxEffect> GetDefaultDamageEffect() const;
    
    /**
     * 지정된 대상에게 CC 효과를 적용합니다.
     * @param SourceASC - CC 효과를 적용한 원본 컴포넌트입니다.
     * @param TargetASC - CC 효과를 적용할 대상 컴포넌트입니다.
     * @param Tag - 적용할 CC 효과를 식별하는 게임플레이 태그입니다.
     * @param Duration - CC 효과의 지속 시간입니다.
     * @param Magnitude - CC 효과의 수치 데이터입니다. (예: 데미지 피해량, 속도 감소 비율 등)
     */
    bool ApplyCrowdControl(UActionSystemComponent* SourceASC, UActionSystemComponent* TargetASC, const FGameplayTag Tag, float Duration, float Magnitude);

    
    /**
    * 외부에서 모든 정보가 주입된 EffectSpec을 받아 대상에게 적용하고 로그를 남깁니다.
    * @param SourceASC - 효과를 발생시킨 컴포넌트입니다.
    * @param TargetASC - 효과를 받을 대상 컴포넌트입니다.
    * @param DamageSpecHandle - 기본 데미지, 스케일링 등 모든 정보가 주입된 이펙트 스펙 핸들입니다.
    */
    bool ApplyDamage(UActionSystemComponent* SourceASC, UActionSystemComponent* TargetASC, const FLuxEffectSpecHandle& DamageSpecHandle);

    /**
     * 전투 로그를 기록합니다.
     * @param Spec - 전투 로그를 기록할 이펙트 스펙입니다.
     * @param FinalDamage - 최종 데미지 값입니다.
     */
    void RecordCombatLog(const struct FLuxEffectSpec& Spec, float FinalDamage);

    /**
     * 전투 로그를 정리합니다.
     * @param MaxEntries - 유지할 최대 로그 항목 수입니다.
     */
    void ClearCombatLog(int32 MaxEntries = 1000);

    /**
     * 특정 시간 범위 내의 전투 로그를 가져옵니다.
     * @param StartTime - 시작 시간 (초)
     * @param EndTime - 종료 시간 (초)
     * @return 해당 시간 범위의 로그 항목들
     */
    TArray<FCombatLogEntry> GetCombatLogInTimeRange(float StartTime, float EndTime) const;

    /**
     * 특정 액터와 관련된 전투 로그를 가져옵니다.
     * @param Actor - 검색할 액터
     * @param bAsDamageCauser - true면 데미지를 입힌 액터로, false면 데미지를 받은 액터로 검색
     * @return 해당 액터와 관련된 로그 항목들
     */
    TArray<FCombatLogEntry> GetCombatLogForActor(const AActor* Actor, bool bAsDamageCauser = true) const;

    /**
     * 전체 전투 로그를 가져옵니다.
     * @return 모든 전투 로그 항목들
     */
    const TArray<FCombatLogEntry>& GetCombatLog() const { return CombatLog; }

    /** 데미지 이벤트를 방송합니다.
     * @param SourceASC - 효과를 발생시킨 컴포넌트입니다.
     * @param TargetASC - 효과를 받을 대상 컴포넌트입니다.
     * @param Spec - 데미지 이벤트를 방송할 이펙트 스펙입니다.
     */
    void BroadcastDamagedEvent(UActionSystemComponent* SourceASC, UActionSystemComponent* TargetASC, const struct FLuxEffectSpec& Spec);

private:
    /* CC 데이터 매핑 맵 */
    UPROPERTY()
    TMap<FGameplayTag, FCrowdControlData> CrowdControlData;

    // 기본 데미지 처리에 사용할 LuxEffect 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Combat Defaults", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<ULuxEffect> DefaultDamageEffectClass;

    /* 전투 로그 */
    UPROPERTY()
    TArray<FCombatLogEntry> CombatLog;
};
