// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuxAssetManager.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"
#include "LuxCueData.h"
#include "LuxGameData.h"
#include "LuxEffectData.h"
#include "LuxCrowdControlData.h"
#include "StatMappingData.h"
#include "Targeting/LuxTargetingData.h"
#include "Targeting/Filters/TargetFilter.h"
#include "ActionSystem/Effects/LuxEffect.h"

#include "AbilitySystemGlobals.h"
#include "Character/LuxPawnData.h"
#include "Misc/App.h"
#include "Stats/StatsMisc.h"
#include "Engine/Engine.h"
#include "Misc/ScopedSlowTask.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxAssetManager)

ULuxAssetManager::ULuxAssetManager()
{
	DefaultPawnData = nullptr;
}


/**
 * 게임 전용으로 설정된 LuxAssetManager 싱글톤을 반환합니다.
 * GEngine->AssetManager가 ULuxAssetManager로 설정되어 있어야 하며,
 * 그렇지 않은 경우 DefaultEngine.ini 설정 오류로 치명적인 로그 처리합니다.
 * Fatal 매크로에서 컴파일 오류를 피하기 위해 NewObject로 생성된 더미 인스턴스를 반환하지만,
 * 이 코드는 실제로 호출되지 않아야 합니다.
 */
ULuxAssetManager& ULuxAssetManager::Get()
{
	check(GEngine);

	// 유일한 AssetManager 인스턴스 반환
	if (ULuxAssetManager* Singleton = Cast<ULuxAssetManager>(GEngine->AssetManager))
		return *Singleton;

	// (실행 불가) 컴파일 오류 회피용 더미 인스턴스
	UE_LOG(LogLux, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini. It must be set to LuxAssetManager!"));
	return *NewObject<ULuxAssetManager>();
}



/**
 * 주어진 소프트 오브젝트 경로의 에셋을 동기적으로 로드합니다.
 * 에셋 매니저가 초기화된 후에는 StreamableManager를 사용하고, 그렇지 않으면 TryLoad로 대체합니다.
 *
 * @param AssetPath    로드할 에셋의 소프트 오브젝트 경로
 * @return             로드된 UObject 포인터 (실패 시 nullptr)
 */
UObject* ULuxAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	// 유효하지 않은 경로일 경우 즉시 반환
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	// 에셋 매니저 초기화 후 StreamableManager를 이용한 로드
	if (UAssetManager::IsInitialized())
	{
		return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
	}

	// 에셋 매니저 미완료 시 TryLoad 사용
	return AssetPath.TryLoad();
}




/**
 * 커맨드라인에서 '-LogAssetLoads' 플래그가 있는지 첫 호출 시에만 검사하여, 이후 호출 시 캐시된 결과를 반환합니다.
 *
 * - FParse::Param: 커맨드라인 문자열에 키워드가 있는지 확인
 * - static 변수: 처음 한 번만 파싱, 이후에는 즉시 반환하여 최적화
 *
 * @return true면 에셋 로드 시 로드 시간 로그를 출력
 */
bool ULuxAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}


/**
 * 로드된 에셋을 추적 목록에 추가하여 가비지 컬렉션으로부터 보호합니다.
 *
 * @param Asset    보호할 UObject 포인터
 */
void ULuxAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (!ensureAlways(Asset))
	{
		return;
	}

	// 멀티스레드 환경을 대비한 크리티컬 섹션 잠금
	FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);

	// 에셋을 세트(Set)에 추가
	LoadedAssets.Add(Asset);
}


/**
 * 초기 로딩 시 필수적인 데이터(GameData, StatMappingData)를 동기적으로 로드하고 초기화합니다.
 */
void ULuxAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// 게임 전역 데이터 로딩
	const ULuxGameData& GameData = GetGameData();
	
	// 각 데이터 타입별로 분리된 함수들을 호출하여 미리 로딩
	PreloadEffectData(GameData);
	PreloadCueData(GameData);
	PreloadCrowdControlData(GameData);
	PreloadTargetingData(GameData);
	
	// 스탯 맵핑 데이터 캐시 초기화
	InitializeStatMappingDataCache();
	
	UE_LOG(LogLux, Log, TEXT("AssetManager 초기 로딩 완료: GameData, EffectData, CueData, CrowdControlData, TargetingData 및 StatMappingData 캐시 초기화됨"));
}

void ULuxAssetManager::RemoveLoadedAsset(const UObject* Asset)
{
	if (!Asset) return;
	FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
	LoadedAssets.Remove(Asset);
}

const ULuxGameData& ULuxAssetManager::GetGameData()
{
	return GetOrLoadTypedGameData<ULuxGameData>(LuxGameDataPath);
}

const UStatMappingData& ULuxAssetManager::GetStatMappingData()
{
	return GetOrLoadTypedGameData<UStatMappingData>(StatMappingDataPath);
}

const ULuxPawnData* ULuxAssetManager::GetDefaultPawnData() const
{
	return GetAsset(DefaultPawnData);
}


/**
 * 지정된 PrimaryDataAsset 클래스를 동기적으로 로드하고 전역 캐시에 추가합니다.
 *
 * - 런타임 환경에서는 LoadPrimaryAssetsWithType으로 비동기 로드 후 완료를 대기합니다.
 * - 로드 후 GameDataMap에 캐시하여 다음 접근 시 GC를 방지하고 즉시 반환합니다.
 *
 * @param DataClass           로드할 UPrimaryDataAsset 서브클래스 타입
 * @param DataClassPath       데이터의 TSoftObjectPtr 경로
 * @param PrimaryAssetType    AssetManager에서 사용하는 PrimaryAssetType 식별자
 * @return                    로드된 UPrimaryDataAsset 인스턴스, 실패 시 nullptr (Fatal 오류)
 */

UPrimaryDataAsset* ULuxAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);

	// 유효하지 않은 경로일 경우 치명적 오류
	if (DataClassPath.IsNull())
	{
		UE_LOG(LogLux, Fatal, TEXT("Missing GameData path for type %s"), *PrimaryAssetType.ToString());
		return nullptr;
	}

	if (GIsEditor)
	{
		Asset = DataClassPath.LoadSynchronous();
		LoadPrimaryAssetsWithType(PrimaryAssetType);
	}
	else
	{
		// 로딩 우선순위를 위해 먼저 에셋 로드를 요청하고 완료를 대기함
		auto Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
		if (Handle.IsValid())
		{
			Handle->WaitUntilComplete(0.0f, false);
			Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
		}
	}

	if (!Asset)
	{
		UE_LOG(LogLux, Error, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable for %s."), *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
		return nullptr;
	}

	// 캐시에 저장
	GameDataMap.Add(DataClass, Asset);
	return Asset;
}

void ULuxAssetManager::PreloadEffectData(const ULuxGameData& GameData)
{
	// EffectData 로딩 및 내부 이펙트 클래스들 미리 로딩
	const ULuxEffectData* EffectData = GetAsset(GameData.EffectData);
	if (EffectData)
	{
		// 모든 이펙트 클래스들을 미리 로딩하여 런타임 지연 방지
		TArray<TSoftClassPtr<ULuxEffect>> EffectClasses = {
			EffectData->InstantDamageEffect,
			EffectData->PeriodicDamageEffect,
			EffectData->InstantHealEffect,
			EffectData->PeriodicHealEffect,
			EffectData->CooldownEffect,
			EffectData->CostEffect,
			EffectData->DynamicTagEffect
		};

		for (const TSoftClassPtr<ULuxEffect>& EffectClass : EffectClasses)
		{
			if (!EffectClass.IsNull())
			{
				UClass* LoadedClass = GetSubclass(EffectClass);
				if (LoadedClass)
				{
					UE_LOG(LogLux, Warning, TEXT("Effect class preloaded: %s"), *LoadedClass->GetName());
				}
				else
				{
					UE_LOG(LogLux, Warning, TEXT("Failed to preload effect class: %s"), *EffectClass.ToString());
				}
			}
		}
	}
	else
	{
		UE_LOG(LogLux, Error, TEXT("Failed to load EffectData during initial loading"));
	}
}

void ULuxAssetManager::PreloadCueData(const ULuxGameData& GameData)
{
	// CueData 로딩 및 내부 데이터 테이블들 미리 로딩
	const ULuxCueData* CueData = GetAsset(GameData.CueData);
	if (CueData)
	{
		// 모든 큐 관련 데이터 테이블들을 미리 로딩하여 런타임 지연 방지
		TArray<TSoftObjectPtr<UDataTable>> CueDataTables = {
			CueData->CueDataTable
			/*CueData->SoundCueDataTable,
			CueData->VisualCueDataTable,
			CueData->ParticleCueDataTable,
			CueData->AnimationCueDataTable*/
		};

		for (const TSoftObjectPtr<UDataTable>& DataTable : CueDataTables)
		{
			if (!DataTable.IsNull())
			{
				UDataTable* LoadedTable = GetAsset(DataTable);
				if (LoadedTable)
				{
					UE_LOG(LogLux, Warning, TEXT("Cue data table preloaded: %s"), *LoadedTable->GetName());
				}
				else
				{
					UE_LOG(LogLux, Warning, TEXT("Failed to preload cue data table: %s"), *DataTable.ToString());
				}
			}
		}
	}
	else
	{
		UE_LOG(LogLux, Error, TEXT("Failed to load CueData during initial loading"));
	}
}

void ULuxAssetManager::PreloadCrowdControlData(const ULuxGameData& GameData)
{
	// CrowdControlData 로딩 및 내부 데이터 테이블들 미리 로딩
	const ULuxCrowdControlData* CrowdControlData = GetAsset(GameData.CrowdControlData);
	if (CrowdControlData)
	{
		// 크라우드 컨트롤 관련 데이터 테이블들을 미리 로딩하여 런타임 지연 방지
		TArray<TSoftObjectPtr<UDataTable>> CrowdControlDataTables = {
			CrowdControlData->CrowdControlTable
		};

		for (const TSoftObjectPtr<UDataTable>& DataTable : CrowdControlDataTables)
		{
			if (!DataTable.IsNull())
			{
				UDataTable* LoadedTable = GetAsset(DataTable);
				if (LoadedTable)
				{
					UE_LOG(LogLux, Warning, TEXT("Crowd control data table preloaded: %s"), *LoadedTable->GetName());
				}
				else
				{
					UE_LOG(LogLux, Warning, TEXT("Failed to preload crowd control data table: %s"), *DataTable.ToString());
				}
			}
		}
	}
	else
	{
		UE_LOG(LogLux, Error, TEXT("Failed to load CrowdControlData during initial loading"));
	}
}

void ULuxAssetManager::PreloadTargetingData(const ULuxGameData& GameData)
{
	// TargetingData 로딩 및 내부 데이터들을 미리 로딩
	const ULuxTargetingData* TargetingData = GetAsset(GameData.TargetingData);
	if (TargetingData)
	{
		// 오버레이 머티리얼들을 미리 로딩
		TArray<TSoftObjectPtr<UMaterialInterface>> OverlayMaterials = {
			TargetingData->HostileOverlayMaterial,
			TargetingData->FriendlyOverlayMaterial,
			TargetingData->NeutralOverlayMaterial
		};

		for (const TSoftObjectPtr<UMaterialInterface>& Material : OverlayMaterials)
		{
			if (!Material.IsNull())
			{
				UMaterialInterface* LoadedMaterial = GetAsset(Material);
				if (LoadedMaterial)
				{
					UE_LOG(LogLux, Warning, TEXT("Targeting overlay material preloaded: %s"), *LoadedMaterial->GetName());
				}
				else
				{
					UE_LOG(LogLux, Warning, TEXT("Failed to preload targeting overlay material: %s"), *Material.ToString());
				}
			}
		}

		UE_LOG(LogLux, Log, TEXT("Targeting data preloaded successfully"));
	}
	else
	{
		UE_LOG(LogLux, Error, TEXT("Failed to load TargetingData during initial loading"));
	}
}

void ULuxAssetManager::InitializeStatMappingDataCache()
{
	// 스탯 맵핑 데이터 로딩 및 캐시 초기화
	const UStatMappingData& StatMapping = GetStatMappingData();
	const_cast<UStatMappingData&>(StatMapping).InitializeCache();
}