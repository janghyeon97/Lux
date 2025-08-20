// Fill out your copyright notice in the Description page of Project Settings.

#include "System/StatMappingData.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "ActionSystem/Attributes/CombatSet.h"
#include "ActionSystem/Attributes/ResourceSet.h"
#include "ActionSystem/Attributes/MovementSet.h"
#include "ActionSystem/Attributes/DefenseSet.h"
#include "System/LuxAssetManager.h"

#include "Engine/DataTable.h"
#include "LuxLogChannels.h"

UStatMappingData::UStatMappingData()
{
   
}

FPrimaryAssetId UStatMappingData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("StatMappingData")), GetFName());
}

void UStatMappingData::PostLoad()
{
	Super::PostLoad();
	
	// PostLoad에서 DataTable을 미리 로드하여 TSoftObjectPtr 문제 해결
	if (!StatMappingTable.IsNull())
	{
		StatMappingTable.LoadSynchronous();
	}
}

const UStatMappingData& UStatMappingData::Get()
{
    return ULuxAssetManager::Get().GetStatMappingData();
}

float UStatMappingData::GetStatValue(const FString& StatName, UActionSystemComponent* ASC) const
{
    if (!ASC)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] ActionSystemComponent가 유효하지 않습니다."), *GetNameSafe(this));
        return 0.0f;
    }

    // 방어 코드: 캐시가 초기화되지 않았으면 초기화
    if (CachedStatMappings.Num() == 0)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] 스탯 맵핑 캐시가 초기화되지 않았습니다. 지연 초기화를 진행합니다."), *GetNameSafe(this));
        InitializeCache();
        
        // 초기화 후에도 빈 상태라면 (데이터 테이블 문제) 즉시 반환
        if (CachedStatMappings.Num() <= 1 && CachedStatMappings.Contains(TEXT("__EMPTY__")))
        {
            UE_LOG(LogLux, Error, TEXT("[%s] 스탯 맵핑 데이터가 없습니다. StatName: %s"), *GetNameSafe(this), *StatName);
            return 0.0f;
        }
    }

    // 축약된 이름을 원래 이름으로 변환
    const FActionTooltipStatMappingRow* StatMapping = CachedStatMappings.Find(StatName);
    if (!StatMapping)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] 스탯 맵핑을 찾을 수 없습니다: %s"), *GetNameSafe(this), *StatName);
        return 0.0f;
    }

    // 커스텀 함수 사용 여부 확인
    if (StatMapping->bUseCustomFunction)
    {
        return GetCustomStatValue(StatName, ASC);
    }

    // 어트리뷰트에서 값 가져오기
    if (StatMapping->AttributeToGet.IsValid())
    {
        const FLuxAttribute& AttributeToGet = StatMapping->AttributeToGet;
        if (!AttributeToGet.IsValid()) return 0.0f;

        const ULuxAttributeSet* Set = ASC->GetAttributeSet(AttributeToGet.GetAttributeSetClass());
        if (!Set) return 0.0f;

        const FLuxAttributeData* Data = AttributeToGet.GetAttributeData(Set);
        if (!Data) return 0.0f;

        return Data->GetCurrentValue();
    }

    // 기본값 반환
    UE_LOG(LogLux, Warning, TEXT("[%s] 스탯 '%s'에 대한 어트리뷰트를 찾을 수 없어 기본값을 반환합니다: %.1f"), 
        *GetNameSafe(this), *StatName, StatMapping->DefaultValue);
    
    return StatMapping->DefaultValue;
}

const FActionTooltipStatMappingRow& UStatMappingData::GetStatMapping(const FString& StatName) const
{
    InitializeCache();
    return *CachedStatMappings.Find(StatName);
}

TArray<FString> UStatMappingData::GetAllStatNames() const
{
    InitializeCache();
    
    TArray<FString> StatNames;
    CachedStatMappings.GetKeys(StatNames);
    return StatNames;
}

void UStatMappingData::InitializeCache() const
{
    // 이미 캐시가 초기화되어 있으면 리턴
    if (CachedStatMappings.Num() > 0)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s] 스탯 맵핑 캐시가 이미 초기화되어 있습니다. (총 %d개)"), *GetNameSafe(this), CachedStatMappings.Num());
        return;
    }



    // TSoftObjectPtr이 설정되어 있지만 로드되지 않은 경우 강제 로드 시도
    UDataTable* DataTable = nullptr;
    if (!StatMappingTable.IsValid())
    {
        if (!StatMappingTable.IsNull())
        {
            UE_LOG(LogLux, Warning, TEXT("[%s] StatMappingTable이 설정되어 있지만 로드되지 않았습니다. 강제 로드를 시도합니다..."), *GetNameSafe(this));
            DataTable = StatMappingTable.LoadSynchronous();
        }
        
        if (!DataTable)
        {
            UE_LOG(LogLux, Error, TEXT("[%s] 스탯 맵핑 데이터 테이블이 설정되지 않았거나 로드에 실패했습니다!"), *GetNameSafe(this));
            UE_LOG(LogLux, Error, TEXT("StatMappingTable 경로: %s"), *StatMappingTable.ToString());
            
            // 캐시를 빈 상태로 표시하여 반복 초기화 방지
            CachedStatMappings.Add(TEXT("__EMPTY__"), FActionTooltipStatMappingRow());
            return;
        }
    }
    else
    {
        DataTable = StatMappingTable.LoadSynchronous();
    }
    if (!DataTable)
    {
        UE_LOG(LogLux, Error, TEXT("[%s] 스탯 맵핑 데이터 테이블 로드에 실패했습니다."), *GetNameSafe(this));
        return;
    }

    // 데이터 테이블에서 모든 행을 가져와서 캐시에 저장
    TArray<FActionTooltipStatMappingRow*> AllRows;
    DataTable->GetAllRows<FActionTooltipStatMappingRow>(TEXT("GetAllStatMappings"), AllRows);

    for (FActionTooltipStatMappingRow* Row : AllRows)
    {
        if (Row)
        {
            // 메인 스탯 이름으로 등록
            CachedStatMappings.Add(Row->StatName, *Row);
            
            // 축약형들도 같은 Row로 등록 (빠른 검색을 위해)
            if (!Row->Abbreviations.IsEmpty())
            {
                TArray<FString> AbbrevList;
                Row->Abbreviations.ParseIntoArray(AbbrevList, TEXT(","));
                
                for (FString& Abbrev : AbbrevList)
                {
                    Abbrev = Abbrev.TrimStartAndEnd(); // 공백 제거
                    if (!Abbrev.IsEmpty())
                    {
                        CachedStatMappings.Add(Abbrev, *Row);
                    }
                }
            }
        }
    }

    UE_LOG(LogLux, Log, TEXT("[%s] 스탯 맵핑 캐시 초기화 완료. 총 %d개 맵핑이 데이터 테이블에서 로드됨"), 
        *GetNameSafe(this), CachedStatMappings.Num());
}

float UStatMappingData::GetCustomStatValue(const FString& StatName, UActionSystemComponent* ASC) const
{
    // 커스텀 스탯 계산 로직 (AttributeSet에 없는 특별한 경우들만)
    if (StatName == TEXT("Level"))
    {
        // TODO: 실제 플레이어 레벨 가져오기
        // 예시:
        // if (APawn* OwnerPawn = ASC->GetOwnerActor<APawn>())
        // {
        //     if (APlayerState* PS = OwnerPawn->GetPlayerState())
        //     {
        //         return PS->GetCharacterLevel();
        //     }
        // }
        return 1.0f;
    }

    UE_LOG(LogLux, Warning, TEXT("[%s] 알 수 없는 커스텀 스탯: %s. 데이터 테이블에서 AttributeToGet을 설정하거나 커스텀 함수를 구현하세요."), *GetNameSafe(this), *StatName);
    return 0.0f;
}

void UStatMappingData::DebugPrintStatus() const
{
    UE_LOG(LogLux, Warning, TEXT("=== StatMappingData 디버그 정보 ==="));
    UE_LOG(LogLux, Warning, TEXT("애셋 이름: %s"), *GetNameSafe(this));
    UE_LOG(LogLux, Warning, TEXT("StatMappingTable.IsValid(): %s"), StatMappingTable.IsValid() ? TEXT("true") : TEXT("false"));
    UE_LOG(LogLux, Warning, TEXT("StatMappingTable 경로: %s"), *StatMappingTable.ToString());
    
    if (StatMappingTable.IsValid())
    {
        UDataTable* DataTable = StatMappingTable.LoadSynchronous();
        if (DataTable)
        {
            UE_LOG(LogLux, Warning, TEXT("DataTable 로드 성공: %s"), *GetNameSafe(DataTable));
            UE_LOG(LogLux, Warning, TEXT("DataTable 행 개수: %d"), DataTable->GetRowMap().Num());
            
            // 몇 개의 행 이름 출력
            int32 Count = 0;
            for (auto& RowPair : DataTable->GetRowMap())
            {
                UE_LOG(LogLux, Warning, TEXT("행 %d: %s"), Count++, *RowPair.Key.ToString());
                if (Count >= 3) break; // 처음 3개만 출력
            }
        }
        else
        {
            UE_LOG(LogLux, Warning, TEXT("DataTable 로드 실패!"));
        }
    }
    
    UE_LOG(LogLux, Warning, TEXT("캐시된 스탯 개수: %d"), CachedStatMappings.Num());
    UE_LOG(LogLux, Warning, TEXT("====================================="));
}
