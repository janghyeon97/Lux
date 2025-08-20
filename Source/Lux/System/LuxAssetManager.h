// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/AssetManager.h"
#include "Templates/SubclassOf.h"
#include "LuxAssetManager.generated.h"

class UPrimaryDataAsset;
class ULuxGameData;
class ULuxPawnData;
class UStatMappingData;
class ULuxCrowdControlData;
class ULuxTargetingData;


UCLASS(Config = Game)
class ULuxAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	ULuxAssetManager();

	/** AssetManager 싱글톤 인스턴스를 반환합니다. */
	static ULuxAssetManager& Get();

	/** TSoftObjectPtr을 사용하는 에셋을 반환합니다. 아직 로드되지 않았다면 동기적으로 로드합니다. */
	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	/** TSoftClassPtr을 사용하는 UClass 서브클래스를 반환합니다. 아직 로드되지 않았다면 동기적으로 로드합니다. */
	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	void RemoveLoadedAsset(const UObject* Asset);

	const ULuxGameData& GetGameData();
	const ULuxPawnData* GetDefaultPawnData() const;
	const UStatMappingData& GetStatMappingData();

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const* pResult = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*pResult);
		}

		return *CastChecked<const GameDataClass>(LoadGameDataOfClass(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	static bool ShouldLogAssetLoads();

	// 로드된 에셋을 추적하고(가비지 컬렉션 방지) 메모리에 유지하도록 추가합니다.
	void AddLoadedAsset(const UObject* Asset);

	//~UAssetManager interface
	virtual void StartInitialLoading() override;
	//~End of UAssetManager interface

	// 지정된 DataClass 타입의 기본 데이터 에셋을 동기적으로 로드합니다.
	UPrimaryDataAsset* LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);

private:
	/** EffectData 및 내부 이펙트 클래스들을 미리 로딩합니다. */
	void PreloadEffectData(const ULuxGameData& GameData);

	/** CueData 및 내부 데이터 테이블들을 미리 로딩합니다. */
	void PreloadCueData(const ULuxGameData& GameData);

	/** CrowdControlData 및 내부 데이터 테이블들을 미리 로딩합니다. */
	void PreloadCrowdControlData(const ULuxGameData& GameData);

	/** TargetingData 및 내부 데이터 테이블들을 미리 로딩합니다. */
	void PreloadTargetingData(const ULuxGameData& GameData);

	/** StatMappingData 캐시를 초기화합니다. */
	void InitializeStatMappingDataCache();

protected:
	// 게임 전체에 대한 데이터가 담긴 에셋 경로입니다.
	UPROPERTY(Config)
	TSoftObjectPtr<ULuxGameData> LuxGameDataPath;

	// 스탯 맵핑 데이터 에셋 경로입니다.
	UPROPERTY(Config)
	TSoftObjectPtr<UStatMappingData> StatMappingDataPath;

	// 로드된 게임 데이터 인스턴스를 저장합니다.
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	// PlayerState에 PawnData가 없을 때 사용할 기본 PawnData 경로입니다.
	UPROPERTY(Config)
	TSoftObjectPtr<ULuxPawnData> DefaultPawnData;

private:
	// Assets loaded and tracked by the asset manager.
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	// Used for a scope lock when modifying the list of load assets.
	FCriticalSection LoadedAssetsCritical;
};



/**
 * 소프트 오브젝트 포인터로 지정된 에셋을 동기적으로 로드하고, 필요 시 메모리에 유지하여 가비지 컬렉션으로부터 보호하는 함수입니다.
 *
 * - AssetPointer에 유효한 경로가 있을 경우에만 로드를 시도합니다.
 * - 이미 로드된 에셋이라면 즉시 반환하고, 아니면 동기적으로 로드합니다.
 * - bKeepInMemory=true일 경우 추적 목록에 추가하여 GC를 방지합니다.
 *
 * @tparam AssetType       로드할 에셋의 타입
 * @param AssetPointer     로드할 에셋의 소프트 오브젝트 포인터
 * @param bKeepInMemory    로드 후 메모리에 계속 유지할지 여부
 * @return                 성공하면 로드된 에셋, 실패 시 nullptr
 */
template<typename AssetType>
AssetType* ULuxAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	// 유효한 경로가 없으면 즉시 중단
	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	// 이미 로드된 에셋이라면 바로 반환
	AssetType* LoadedAsset = AssetPointer.Get();

	// 아직 로드되지 않았다면 동기 로드 시도
	if (!LoadedAsset)
	{
		LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
		ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
	}

	// 메모리 유지 옵션이 켜졌다면 추적 목록에 추가
	if (LoadedAsset && bKeepInMemory)
	{
		Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
	}

	return LoadedAsset;
}



/**
 * 소프트 클래스 포인터로 지정된 UClass 서브클래스를 동기적으로 로드하고, 필요 시 메모리에 유지하여 가비지 컬렉션으로부터 보호하는 함수입니다.
 *
 * - AssetPointer에 유효한 경로가 있을 경우에만 로드를 시도합니다.
 * - 이미 로드된 서브클래스라면 즉시 반환하고, 아니면 동기적으로 로드합니다.
 * - bKeepInMemory=true일 경우 추적 목록에 추가하여 GC로부터 보호합니다.
 *
 * @tparam AssetType       로드할 클래스가 상속하는 타입
 * @param AssetPointer     로드할 에셋의 소프트 클래스 포인터
 * @param bKeepInMemory    로드 후 서브클래스를 메모리에 계속 유지할지 여부
 * @return                 성공하면 로드된 TSubclassOf<AssetType>, 실패 시 nullptr
 */
template<typename AssetType>
TSubclassOf<AssetType> ULuxAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	// 에셋 클래스 경로 획득
	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	// 이미 로드된 서브클래스라면 바로 반환
	TSubclassOf<AssetType> LoadedSubclass = AssetPointer.Get();

	// 아직 로드되지 않았다면 동기 로드 시도
	if (!LoadedSubclass)
	{
		LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath)); // 동기 로드
		ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
	}

	// 유지 옵션이 켜졌다면 추적 목록에 추가하여 GC 방지
	if (LoadedSubclass && bKeepInMemory)
	{
		Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
	}

	return LoadedSubclass;
}

