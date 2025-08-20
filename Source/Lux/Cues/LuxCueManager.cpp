// Fill out your copyright notice in the Description page of Project Settings.


#include "Cues/LuxCueManager.h"
#include "System/LuxAssetManager.h"
#include "System/LuxGameData.h"
#include "LuxLogChannels.h"
#include "Engine/DataTable.h"


void ULuxCueManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ULuxAssetManager& AssetManager = ULuxAssetManager::Get();
	const ULuxGameData& GameData = AssetManager.GetGameData();

	// CueData에서 CueDataTable 로드
	const ULuxCueData* CueData = AssetManager.GetAsset(GameData.CueData);
	if (!CueData)
	{
		UE_LOG(LogLux, Error, TEXT("ULuxCueManager: Failed to load CueData from GameData"));
		return;
	}

	UDataTable* LoadedDataTable = AssetManager.GetAsset(CueData->CueDataTable);
	if (!LoadedDataTable)
	{
		UE_LOG(LogLux, Error, TEXT("ULuxCueManager: Failed to load CueDataTable from CueData"));
		return;
	}

	TArray<FLuxCueDataRow*> AllRows;
	LoadedDataTable->GetAllRows(TEXT(""), AllRows);

	for (const FLuxCueDataRow* Row : AllRows)
	{
		if (Row && Row->CueTag.IsValid() && Row->CueClass)
		{
			CueClassMap.Add(Row->CueTag, Row->CueClass);
		}
	}

	// CueClassMap을 기반으로 오브젝트 풀을 생성합니다.
	InitializeCuePool();
}

void ULuxCueManager::Deinitialize()
{
	for (auto& Pair : CuePools)
	{
		for (ALuxCueNotify* Cue : Pair.Value.Items)
		{
			if (Cue)
			{
				Cue->Destroy();
			}
		}
	}

	CuePools.Empty();

	TArray<ALuxCueNotify*> AllActiveCues;
	for (auto& TargetPair : ActiveCues)
	{
		for (auto& CueTagPair : TargetPair.Value.CueMap)
		{
			AllActiveCues.Append(CueTagPair.Value.Cues);
		}
	}

	for (ALuxCueNotify* Cue : AllActiveCues)
	{
		if (Cue)
		{
			Cue->Stop();
		}
	}

	ActiveCues.Empty();

	Super::Deinitialize();
}

ALuxCueNotify* ULuxCueManager::SpawnNewCue(FGameplayTag CueTag, const FTransform& SpawnTransform)
{
	if (!GetWorld()) return nullptr;

	const TSubclassOf<ALuxCueNotify>* CueClassPtr = CueClassMap.Find(CueTag);
	if (!CueClassPtr || !*CueClassPtr)
	{
		UE_LOG(LogLux, Error, TEXT("Failed to find CueDataRow or spawn a Cue for tag '%s'."), *CueTag.ToString());
		return nullptr;
	}

	ALuxCueNotify* NewCue = GetWorld()->SpawnActor<ALuxCueNotify>(*CueClassPtr, SpawnTransform);
	return ::IsValid(NewCue) ? NewCue : nullptr;
}

void ULuxCueManager::InitializeCuePool()
{
	if (CueClassMap.IsEmpty()) return;

	for (const auto& Pair : CueClassMap)
	{
		const FGameplayTag& CueTag = Pair.Key;
		const TSubclassOf<ALuxCueNotify> CueClass = Pair.Value;

		if (!CueClass) continue;

		// CDO가 없거나 풀링을 사용하지 않는 큐는 미리 생성하지 않습니다.
		const ALuxCueNotify* CDO = CueClass->GetDefaultObject<ALuxCueNotify>();
		if (!CDO || CDO->bAutoDestroyOnRemove) continue;

		FLuxCueNotifyPool& Pool = CuePools.FindOrAdd(CueTag);
		for (int32 i = 0; i < CDO->PoolSize; ++i)
		{
			if (ALuxCueNotify* NewCue = SpawnNewCue(CueTag, FTransform::Identity))
			{
				NewCue->SetActorHiddenInGame(true);
				NewCue->SetActorTickEnabled(false);
				Pool.Items.Add(NewCue);
			}
		}
	}
}

void ULuxCueManager::OnCueStopped(ALuxCueNotify* Cue)
{
	if (!Cue) return;

	TWeakObjectPtr<AActor> TargetWeakPtr = Cue->CueTarget;
	if (TargetWeakPtr.IsValid())
	{
		// Target에 해당하는 큐 목록을 찾습니다.
		if (FActiveCueNotifyMap* TargetCues = ActiveCues.Find(TargetWeakPtr))
		{
			// 해당 태그의 큐 배열을 찾습니다.
			if (FLuxCueNotifyArray* CueArrayStruct = TargetCues->CueMap.Find(Cue->CueTag))
			{
				CueArrayStruct->Cues.Remove(Cue);

				if (CueArrayStruct->Cues.IsEmpty())
				{
					TargetCues->CueMap.Remove(Cue->CueTag);
				}
			}

			if (TargetCues->CueMap.IsEmpty())
			{
				ActiveCues.Remove(TargetWeakPtr);
			}
		}
	}

	if (Cue->bAutoDestroyOnRemove)
	{
		// 풀링 대상이 아니므로 즉시 파괴합니다.
		Cue->Destroy();
	}
	else
	{
		// 풀링 대상이므로 상태를 리셋하고 풀에 반납합니다.
		Cue->CueTarget = nullptr;
		Cue->OnCueStopped.Unbind();
		Cue->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Cue->SetActorHiddenInGame(true);
		Cue->SetActorTickEnabled(false);
		Cue->SetActorTransform(FTransform::Identity);

		FLuxCueNotifyPool& Pool = CuePools.FindOrAdd(Cue->CueTag);
		Pool.Items.Add(Cue);
	}
}

void ULuxCueManager::HandleCue(AActor* Target, FGameplayTag CueTag, const FLuxCueContext& Context)
{
	if (!Target || !CueTag.IsValid()) return;

	const FString ContextString = GetClientServerContextString(Target);
	UE_LOG(LogLux, Log, TEXT("[%s][CueManager::HandleCue] Received request to handle cue. CueTag: %s, Target: %s"), *ContextString, *CueTag.ToString(), *GetNameSafe(Target));

	ALuxCueNotify* CueToExecute = nullptr;

	// 풀에서 사용 가능한 큐를 가져옵니다.
	if (FLuxCueNotifyPool* Pool = CuePools.Find(CueTag))
	{
		if (!Pool->Items.IsEmpty())
		{
			CueToExecute = Pool->Items.Pop();
			if (CueToExecute)
			{
				CueToExecute->SetActorTransform(FTransform(Context.Rotation, Context.Location, FVector::OneVector));
			}
		}
	}

	// 풀에 큐가 없다면 새로 생성합니다.
	if (!CueToExecute)
	{
		UE_LOG(LogLux, Log, TEXT("[%s][CueManager] No available cue in pool for tag '%s'. Attempting to spawn a new instance."), *ContextString, *CueTag.ToString());
		const FTransform SpawnTransform(Context.Rotation, Context.Location, FVector::OneVector);
		CueToExecute = SpawnNewCue(CueTag, SpawnTransform);
	}

	if (!CueToExecute)
	{
		UE_LOG(LogLux, Error, TEXT("[%s][CueManager] FAILED TO CREATE a new cue instance for tag '%s'. The cue will not be executed."), *ContextString, *CueTag.ToString());
		return;
	}

	UE_LOG(LogLuxActionSystem, Log, TEXT("[%s][CueManager] Cue '%s' is ready. Adding to active list and executing."), *ContextString, *GetNameSafe(CueToExecute));

	// 큐를 활성화 목록에 추가하고 실행합니다.
	FActiveCueNotifyMap& TargetCues = ActiveCues.FindOrAdd(Target);
	FLuxCueNotifyArray& CueArrayStruct = TargetCues.CueMap.FindOrAdd(CueTag);
	CueArrayStruct.Cues.Add(CueToExecute);

	CueToExecute->OnCueStopped.BindUObject(this, &ULuxCueManager::OnCueStopped);
	CueToExecute->Execute(Target, Context);
}

void ULuxCueManager::StopCue(AActor* Target, FGameplayTag CueTag)
{
	if (!Target || !CueTag.IsValid()) return;

	FActiveCueNotifyMap* TargetCues = ActiveCues.Find(Target);
	if (!TargetCues) return;

	FLuxCueNotifyArray* CueArrayStruct = TargetCues->CueMap.Find(CueTag);

	// 배열에서 가장 먼저 추가된 큐를 중지합니다..
	if (CueArrayStruct && CueArrayStruct->Cues.Num() > 0)
	{
		ALuxCueNotify* CueToStop = CueArrayStruct->Cues[0];
		if (CueToStop)
		{
			CueToStop->Stop();
		}
	}
}

void ULuxCueManager::StopAllCuesByTag(AActor* Target, FGameplayTag CueTag)
{
	if (!Target || !CueTag.IsValid()) return;

	FActiveCueNotifyMap* TargetCues = ActiveCues.Find(Target);
	if (!TargetCues) return;

	FLuxCueNotifyArray* CueArrayStruct = TargetCues->CueMap.Find(CueTag);
	if (!CueArrayStruct) return;

	TArray<ALuxCueNotify*> CuesToStop = CueArrayStruct->Cues;
	for (ALuxCueNotify* Cue : CuesToStop)
	{
		if (!Cue) continue;
		Cue->Stop();
	}
}

void ULuxCueManager::StopAllCuesForTarget(AActor* Target)
{
	if (!Target) return;

	TWeakObjectPtr<AActor> TargetWeakPtr = Target;
	FActiveCueNotifyMap* TargetCues = ActiveCues.Find(TargetWeakPtr);
	if (!TargetCues) return;

	const TMap<FGameplayTag, FLuxCueNotifyArray> CuesMapCopy = TargetCues->CueMap;
	for (const auto& Pair : CuesMapCopy)
	{
		const TArray<TObjectPtr<ALuxCueNotify>> CuesToStop = Pair.Value.Cues;
		for (ALuxCueNotify* Cue : CuesToStop)
		{
			if (Cue) Cue->Stop();
		}
	}
}

const TMap<FGameplayTag, FLuxCueNotifyArray>* ULuxCueManager::GetActiveCuesForTarget(AActor* Target) const
{
	if (!Target) return nullptr;

	const FActiveCueNotifyMap* TargetCues = ActiveCues.Find(Target);
	return TargetCues ? &TargetCues->CueMap : nullptr;
}

const TArray<TObjectPtr<ALuxCueNotify>>* ULuxCueManager::GetCuesByTag(AActor* Target, FGameplayTag CueTag) const
{
	const TMap<FGameplayTag, FLuxCueNotifyArray>* CuesMap = GetActiveCuesForTarget(Target);
	if (!CuesMap) return nullptr;
	
	const FLuxCueNotifyArray* CueArray = CuesMap->Find(CueTag);
	if (!CueArray || !(&CueArray->Cues)) nullptr;

	return &CueArray->Cues;
}

int32 ULuxCueManager::GetTotalActiveCueCount() const
{
	int32 TotalCount = 0;
	for (const auto& Pair : ActiveCues)
	{
		for (const auto& InnerPair : Pair.Value.CueMap)
		{
			TotalCount += InnerPair.Value.Cues.Num();
		}
	}
	return TotalCount;
}

int32 ULuxCueManager::GetActiveCueCountForTarget(AActor* Target) const
{
	int32 Count = 0;
	if (const FActiveCueNotifyMap* TargetCues = ActiveCues.Find(Target))
	{
		for (const auto& Pair : TargetCues->CueMap)
		{
			Count += Pair.Value.Cues.Num();
		}
	}
	return Count;
}

int32 ULuxCueManager::GetCueCountByTag(AActor* Target, FGameplayTag CueTag) const
{
	const TArray<TObjectPtr<ALuxCueNotify>>* CueArray = GetCuesByTag(Target, CueTag);
	if (!CueArray) return 0;

	return CueArray->Num();
}

bool ULuxCueManager::HasCue(AActor* Target, FGameplayTag CueTag) const
{
	return GetCueCountByTag(Target, CueTag) > 0;
}