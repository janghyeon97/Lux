// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "LuxPawnDataManagerComponent.generated.h"

class ULuxPawnData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPawnDataLoaded, const ULuxPawnData* /*PawnData*/);

enum class EPawnDataLoadState
{
	Unloaded,
	Loading,
	Loaded,
	Deactivating
};

UCLASS()
class ULuxPawnDataManagerComponent final : public UGameStateComponent
{
	GENERATED_BODY()

public:
	ULuxPawnDataManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	void StartPawnDataLoad(FPrimaryAssetId PawnDataId);

	void CallOrRegister_OnPawnDataLoaded_HighPriority(FPrimaryAssetId PawnDataId, FOnPawnDataLoaded::FDelegate&& Delegate);
	void CallOrRegister_OnPawnDataLoaded(FPrimaryAssetId PawnDataId, FOnPawnDataLoaded::FDelegate&& Delegate);
	void CallOrRegister_OnPawnDataLoaded_LowPriority(FPrimaryAssetId PawnDataId, FOnPawnDataLoaded::FDelegate&& Delegate);
	void UnregisterDelegates(UObject* BoundObject);

	const ULuxPawnData* GetPawnDataChecked(FPrimaryAssetId PawnDataId) const;
	bool IsPawnDataLoaded(FPrimaryAssetId PawnDataId) const;

private:
	UFUNCTION()
	void OnRep_CurrentPawnData();

	void OnPawnDataLoadComplete(FPrimaryAssetId LoadedId);

private:
	// PawnDataId의 로드 상태
	TMap<FPrimaryAssetId, EPawnDataLoadState> LoadStateMap;

	// PawnDataId와 로드된 인스턴스
	TMap<FPrimaryAssetId, const ULuxPawnData*> LoadedDataMap;

	int32 NumGameFeaturePluginsLoading = 0;
	TArray<FString> GameFeaturePluginURLs;

	int32 NumObservedPausers = 0;
	int32 NumExpectedPausers = 0;

	/**
	 * Delegate called when the PawnData has finished loading just before others
	 * (e.g., subsystems that set up for regular gameplay)
	 */
	FOnPawnDataLoaded OnPawnDataLoaded_HighPriority;

	/** Delegate called when the PawnData has finished loading */
	FOnPawnDataLoaded OnPawnDataLoaded;

	/** Delegate called when the PawnData has finished loading */
	FOnPawnDataLoaded OnPawnDataLoaded_LowPriority;
};
