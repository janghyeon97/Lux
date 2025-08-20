// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "StatMappingData.generated.h"

class ULuxEffect;
class UObject; 
class UActionSystemComponent;

/**
 * 액션 툴팁에서 사용할 스탯 맵핑 정보를 정의하는 구조체
 */
USTRUCT(BlueprintType)
struct LUX_API FActionTooltipStatMappingRow : public FTableRowBase
{
    GENERATED_BODY()

    FActionTooltipStatMappingRow()
        : StatName("")
        , Abbreviations("")
        , AttributeToGet()
        , bUseCustomFunction(false)
        , DefaultValue(0.0f)
    {
    }

    /** 메인 스탯 이름 (고유 식별자, 예: "AttackDamage", "Health") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Mapping")
    FString StatName;

    /** 축약된 별칭들 (쉼표로 구분, 예: "AD,공격력,ATK,물리공격력") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Mapping")
    FString Abbreviations;

    /** 실제 가져올 어트리뷰트 (예: CombatSet.AttackDamage) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Mapping")
    FLuxAttribute AttributeToGet;

    /** 커스텀 함수를 사용할지 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Mapping")
    bool bUseCustomFunction;

    /** 어트리뷰트를 찾을 수 없을 때 사용할 기본값 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Mapping")
    float DefaultValue;
};

/**
 * 스탯 맵핑 데이터를 관리하는 데이터 애셋
 */
UCLASS(BlueprintType, Blueprintable)
class LUX_API UStatMappingData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UStatMappingData();

    //~UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~End of UPrimaryDataAsset interface

	//~UObject interface
	virtual void PostLoad() override;
	//~End of UObject interface

    /** AssetManager를 통해 글로벌 인스턴스를 반환합니다. */
    static const UStatMappingData& Get();

    /** 스탯 이름으로 값을 가져옵니다. (축약 지원) */
    UFUNCTION(BlueprintCallable, Category = "Stat Mapping")
    float GetStatValue(const FString& StatName, UActionSystemComponent* ASC) const;

    /** 스탯 맵핑 정보를 가져옵니다. */
    UFUNCTION(BlueprintCallable, Category = "Stat Mapping")
    const FActionTooltipStatMappingRow& GetStatMapping(const FString& StatName) const;

    /** 모든 사용 가능한 스탯 이름들을 가져옵니다. */
    UFUNCTION(BlueprintCallable, Category = "Stat Mapping")
    TArray<FString> GetAllStatNames() const;

public:
    /** 캐시를 초기화합니다. (AssetManager에서 호출용) */
    void InitializeCache() const;

    /** 디버깅: StatMappingData 상태를 출력합니다. */
    void DebugPrintStatus() const;

protected:
    /** 커스텀 스탯 값 계산 함수들 */
    float GetCustomStatValue(const FString& StatName, UActionSystemComponent* ASC) const;

public:
    /** 스탯 맵핑 데이터 테이블 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (RequiredAssetDataTags = "RowStructure=ActionTooltipStatMappingRow"))
    TSoftObjectPtr<UDataTable> StatMappingTable;

    /** 캐시된 스탯 맵핑 정보 */
    UPROPERTY(Transient)
    mutable TMap<FString, FActionTooltipStatMappingRow> CachedStatMappings;
};
