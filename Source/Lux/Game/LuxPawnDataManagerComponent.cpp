// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuxPawnDataManagerComponent.h"
#include "Character/LuxPawnData.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "System/LuxAssetManager.h"
#include "TimerManager.h"
#include "LuxLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxPawnDataManagerComponent)


ULuxPawnDataManagerComponent::ULuxPawnDataManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void ULuxPawnDataManagerComponent::CallOrRegister_OnPawnDataLoaded_HighPriority(FPrimaryAssetId PawnDataId, FOnPawnDataLoaded::FDelegate&& Delegate)
{
	if (IsPawnDataLoaded(PawnDataId))
	{
		const ULuxPawnData** DataPtr = LoadedDataMap.Find(PawnDataId);
		Delegate.Execute(*DataPtr);
	}
	else
	{
		OnPawnDataLoaded_HighPriority.Add(MoveTemp(Delegate));
	}
}

void ULuxPawnDataManagerComponent::CallOrRegister_OnPawnDataLoaded(FPrimaryAssetId PawnDataId, FOnPawnDataLoaded::FDelegate&& Delegate)
{
	if (IsPawnDataLoaded(PawnDataId))
	{
		const ULuxPawnData** DataPtr = LoadedDataMap.Find(PawnDataId);
		Delegate.Execute(*DataPtr);
	}
	else
	{
		OnPawnDataLoaded.Add(MoveTemp(Delegate));
	}
}

void ULuxPawnDataManagerComponent::CallOrRegister_OnPawnDataLoaded_LowPriority(FPrimaryAssetId PawnDataId, FOnPawnDataLoaded::FDelegate&& Delegate)
{
	if (IsPawnDataLoaded(PawnDataId))
	{
		const ULuxPawnData** DataPtr = LoadedDataMap.Find(PawnDataId);
		Delegate.Execute(*DataPtr);
	}
	else
	{
		OnPawnDataLoaded_LowPriority.Add(MoveTemp(Delegate));
	}
}

void ULuxPawnDataManagerComponent::UnregisterDelegates(UObject* BoundObject)
{
	if (!BoundObject)
	{
		return;
	}

	OnPawnDataLoaded_HighPriority.RemoveAll(BoundObject);
	OnPawnDataLoaded.RemoveAll(BoundObject);
	OnPawnDataLoaded_LowPriority.RemoveAll(BoundObject);
}

const ULuxPawnData* ULuxPawnDataManagerComponent::GetPawnDataChecked(FPrimaryAssetId PawnDataId) const
{
	// 1) LoadStateMap에서 상태를 찾아 Loaded 상태인지 검사
	const EPawnDataLoadState* StatePtr = LoadStateMap.Find(PawnDataId);
	check(StatePtr && *StatePtr == EPawnDataLoadState::Loaded);

	// 2) LoadedDataMap에서 실제 인스턴스 데이터를 가져오며 null 체크
	const ULuxPawnData* const* DataPtr = LoadedDataMap.Find(PawnDataId);
	check(DataPtr && *DataPtr);

	// 3) 성공적으로 반환
	return *DataPtr;
}

bool ULuxPawnDataManagerComponent::IsPawnDataLoaded(FPrimaryAssetId PawnDataId) const
{
	if (const EPawnDataLoadState* State = LoadStateMap.Find(PawnDataId))
	{
		return (*State == EPawnDataLoadState::Loaded);
	}

	return false;
}

void ULuxPawnDataManagerComponent::OnRep_CurrentPawnData()
{

}

void ULuxPawnDataManagerComponent::StartPawnDataLoad(FPrimaryAssetId PawnDataId)
{
	// 1) 이미 로드 중이거나 로드된 상태이면 중복 실행 방지
	EPawnDataLoadState& ExistingState = LoadStateMap.FindOrAdd(PawnDataId, EPawnDataLoadState::Unloaded);
	if (ExistingState != EPawnDataLoadState::Unloaded)
	{
		return;
	}

	// 2) 로딩 상태로 변경, 중복 로드 방지
	ExistingState = EPawnDataLoadState::Loading;

	UE_LOG(LogLux, Warning, TEXT("[%s] PawnData '%s' 로드를 시작합니다."), ANSI_TO_TCHAR(__FUNCTION__), *PawnDataId.ToString());

	ULuxAssetManager& AssetManager = ULuxAssetManager::Get();

	// 3) PrimaryAssetId를 SoftObjectPtr로 변환
	FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PawnDataId);
	TSoftObjectPtr<ULuxPawnData> PawnDataPtr(AssetPath);

	// 4) 동기 로드 (Soft Object Pointer 사용)
	const ULuxPawnData* LoadedPawnData = AssetManager.GetAsset<ULuxPawnData>(PawnDataPtr);
	if (!LoadedPawnData)
	{
		UE_LOG(LogLux, Error, TEXT("[%s] PawnData '%s' 로드 실패, 기본 PawnData를 사용합니다."), ANSI_TO_TCHAR(__FUNCTION__), *PawnDataId.ToString());
		LoadedPawnData = AssetManager.GetDefaultPawnData();
	}

	// 5) 비동기 완료 처리를 위한 델리게이트 생성
	FStreamableDelegate Delegate = FStreamableDelegate::CreateLambda([this, PawnDataId]()
		{
			OnPawnDataLoadComplete(PawnDataId);
		}
	);

	AssetManager.LoadPrimaryAssets({ PawnDataId }, /*Bundles=*/{}, Delegate);
}

void ULuxPawnDataManagerComponent::OnPawnDataLoadComplete(FPrimaryAssetId LoadedId)
{
	// 1) 잘못된 상태에서의 호출 방지
	EPawnDataLoadState* StatePtr = LoadStateMap.Find(LoadedId);
	if (!ensureAlwaysMsgf(StatePtr && *StatePtr == EPawnDataLoadState::Loading, TEXT("OnGlobalPawnDataLoaded called with invalid state for %s: %d"), *LoadedId.ToString(), StatePtr ? static_cast<int32>(*StatePtr) : -1))
	{
		return;
	}

	// 2) 로드된 PawnData의 유효성 검사
	ULuxAssetManager& AM = ULuxAssetManager::Get();
	TSoftObjectPtr<ULuxPawnData> SoftPtr(AM.GetPrimaryAssetPath(LoadedId));
	const ULuxPawnData* Data = AM.GetAsset<ULuxPawnData>(SoftPtr);
	if (!ensureAlwaysMsgf(Data, TEXT("Failed to retrieve PawnData asset for %s"), *LoadedId.ToString()))
	{
		return;
	}

	// 3) 로드 완료 상태로 업데이트
	LoadedDataMap.Add(LoadedId, Data);
	*StatePtr = EPawnDataLoadState::Loaded;

	UE_LOG(LogLux, Warning, TEXT("[%s] PawnData '%s' 로드 완료, %s"), ANSI_TO_TCHAR(__FUNCTION__), *Data->GetPrimaryAssetId().ToString(), *GetClientServerContextString(this));

	// 4) High Priority, Normal, Low Priority 순서로 로드 완료 델리게이트를 브로드캐스트
	OnPawnDataLoaded_HighPriority.Broadcast(Data);
	OnPawnDataLoaded_HighPriority.Clear();

	OnPawnDataLoaded.Broadcast(Data);
	OnPawnDataLoaded.Clear();

	OnPawnDataLoaded_LowPriority.Broadcast(Data);
	OnPawnDataLoaded_LowPriority.Clear();
}

void ULuxPawnDataManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);


}