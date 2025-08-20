
#include "LuxActionTypes.h"
#include "LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "LuxLogChannels.h"

void FLuxActionSpecHandle::GenerateNewHandle()
{
	static int32 GHandle = 1;
	Handle = GHandle++;
}

bool FLuxActionSpecHandle::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Handle;
	bOutSuccess = true;
	return true;
}



FLuxActionSpec::FLuxActionSpec()
	: Action(nullptr)
	, Level(0)
	, ActivationCount(0)
{
	
}

FLuxActionSpec::FLuxActionSpec(TSubclassOf<ULuxAction> ActionClass, const FGameplayTag& InInputTag, int32 InLevel)
	: Action(ActionClass ? ActionClass->GetDefaultObject<ULuxAction>() : nullptr)
	, InputTag(InInputTag.IsValid() ? InInputTag : FGameplayTag())
	, ActionIdentifierTag(ActionClass ? ActionClass->GetDefaultObject<ULuxAction>()->ActionIdentifierTag : FGameplayTag())
	, Level(InLevel > 0 ? InLevel : 0)
	, ActivationCount(0)
{
	Handle.GenerateNewHandle();
	DynamicTags.AppendTags(ActionClass ? ActionClass->GetDefaultObject<ULuxAction>()->ActionTags : FGameplayTagContainer());
	DynamicTags.AddTag(InputTag);
}

FLuxActionSpec::FLuxActionSpec(ULuxAction* InAction, const FGameplayTag& InInputTag, int32 InLevel)
	: Action(InAction->GetClass()->GetDefaultObject<ULuxAction>())
	, InputTag(InInputTag)
	, ActionIdentifierTag(InAction ? InAction->ActionIdentifierTag : FGameplayTag())
	, Level(InLevel)
	, ActivationCount(0)
{
	Handle.GenerateNewHandle();
	DynamicTags.AppendTags(InAction ? InAction->ActionTags : FGameplayTagContainer());
	DynamicTags.AddTag(InputTag);
}

FGameplayTag FLuxActionSpec::GetCooldownTag() const
{
	if (!InputTag.IsValid())
	{
		return FGameplayTag();
	}

	FString TagString = InputTag.ToString();
	FString Suffix;

	// "Input.Action.Primary"에서 마지막 "." 뒤의 "Primary"를 추출합니다.
	if (TagString.Split(TEXT("."), nullptr, &Suffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		FString CooldownTagString = FString::Printf(TEXT("Action.Cooldown.%s"), *Suffix);
		return FGameplayTag::RequestGameplayTag(FName(*CooldownTagString), false);
	}

	return FGameplayTag();
}

FGameplayTag FLuxActionSpec::GetStackTag() const
{
	if (!InputTag.IsValid())
	{
		return FGameplayTag();
	}

	FString TagString = InputTag.ToString();
	FString Suffix;

	// "Input.Action.Primary"에서 마지막 "." 뒤의 "Primary"를 추출합니다.
	if (TagString.Split(TEXT("."), nullptr, &Suffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		FString ChargeTagString = FString::Printf(TEXT("Action.Stack.%s"), *Suffix);
		return FGameplayTag::RequestGameplayTag(FName(*ChargeTagString), false);
	}

	return FGameplayTag();
}

FGameplayTag FLuxActionSpec::GetMultiCastCountTag() const
{
	if (!InputTag.IsValid())
	{
		return FGameplayTag();
	}

	// "Input.Action.Primary"에서 마지막 "." 뒤의 "Primary"를 추출합니다.
	FString TagString = InputTag.ToString();
	FString Suffix;
	if (TagString.Split(TEXT("."), nullptr, &Suffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		FString MultiCastTagString = FString::Printf(TEXT("Action.MultiCast.%s"), *Suffix);
		return FGameplayTag::RequestGameplayTag(FName(*MultiCastTagString), false);
	}

	return FGameplayTag();
}

bool FLuxActionSpec::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

	Ar << Action;
	Ar << Handle.Handle;

	InputTag.NetSerialize(Ar, Map, bOutSuccess);
	if (!bOutSuccess) return false;

	ActionIdentifierTag.NetSerialize(Ar, Map, bOutSuccess);
	if (!bOutSuccess) return false;

	DynamicTags.NetSerialize(Ar, Map, bOutSuccess);
	if (!bOutSuccess) return false;

	Ar << Level;
	Ar << InputPressed;
	Ar << ActivationCount;
	Ar << LastExecutionTime;

	return bOutSuccess;
}

void FLuxActionSpec::PreReplicatedRemove(const struct FActionSpecContainer& InArraySerializer)
{
	// UE_LOG(LogTemp, Log, TEXT("FLuxActionSpec::PreReplicatedRemove - Action: %s"), Action ? *Action->GetName() : TEXT("NULL"));
}

void FLuxActionSpec::PostReplicatedAdd(const struct FActionSpecContainer& InArraySerializer)
{
	// UE_LOG(LogTemp, Log, TEXT("FLuxActionSpec::PostReplicatedAdd - Action: %s"), Action ? *Action->GetName() : TEXT("NULL"));
}

void FLuxActionSpec::PostReplicatedChange(const struct FActionSpecContainer& InArraySerializer)
{
	// UE_LOG(LogTemp, Log, TEXT("FLuxActionSpec::PostReplicatedChange - Action: %s"), Action ? *Action->GetName() : TEXT("NULL"));
}

void FActionSpecContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid())
		return;

	for (const int32 Index : AddedIndices)
	{
		FLuxActionSpec& AddedSpec = Items[Index];
		if (!AddedSpec.Action) continue;

		UE_LOG(LogLuxActionSystem, Log, TEXT("[Client] ActionSpec Added: '%s'"), *AddedSpec.Action->GetName());

		//OwnerComponent->ActionSpecMap.Add(AddedSpec.Handle, &AddedSpec);
		for (const FGameplayTag& TriggerTag : AddedSpec.Action->EventTriggerTags)
		{
			OwnerComponent->EventTriggerMap.FindOrAdd(TriggerTag).Add(AddedSpec.Handle);
		}
	}
}

void FActionSpecContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid())
		return;

	for (const int32 Index : ChangedIndices)
	{
		FLuxActionSpec& ChangedSpec = Items[Index];
		if (!ChangedSpec.Action) continue;

		UE_LOG(LogLuxActionSystem, Log, TEXT("[Client] ActionSpec Changed: '%s'"), *ChangedSpec.Action->GetName());
	}
}

void FActionSpecContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid())
		return;

	FRWScopeLock WriteLock(OwnerComponent->ActionSpecsLock, FRWScopeLockType::SLT_Write);
	for (const int32 Index : RemovedIndices)
	{
		FLuxActionSpec& RemovedSpec = Items[Index];
		if (!RemovedSpec.Action) continue;

		UE_LOG(LogLuxActionSystem, Log, TEXT("[Client] ActionSpec Removed: '%s'"), *RemovedSpec.Action->GetName());

		//OwnerComponent->ActionSpecMap.Remove(RemovedSpec.Handle);
		for (const FGameplayTag& TriggerTag : RemovedSpec.Action->EventTriggerTags)
		{
			if (TArray<FLuxActionSpecHandle>* Handles = OwnerComponent->EventTriggerMap.Find(TriggerTag))
			{
				Handles->Remove(RemovedSpec.Handle);
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------

FActiveLuxActionHandle::FActiveLuxActionHandle()
	: Handle(-1)
{
}

bool FActiveLuxActionHandle::IsValid() const
{
	return Handle > 0;
}

FActiveLuxActionHandle FActiveLuxActionHandle::GenerateNewHandle()
{
	FActiveLuxActionHandle NewHandle;
	NewHandle.Handle = ++Counter;
	if (Counter <= 0)
	{
		Counter = 1;
	}

	NewHandle.Handle = Counter;
	return NewHandle;
}

bool FActiveLuxActionHandle::operator==(const FActiveLuxActionHandle& Other) const
{
	return Handle == Other.Handle;
}

bool FActiveLuxActionHandle::operator!=(const FActiveLuxActionHandle& Other) const
{
	return Handle != Other.Handle;
}


FActiveLuxAction::FActiveLuxAction(const FLuxActionSpec& InSpec, const FLuxPredictionKey& InPredictionKey, const FOwningActorInfo& InActorInfo)
	: Spec(InSpec)
	, PredictionKey(InPredictionKey)
	, ActorInfo(InActorInfo)
	, bIsDone(false)
{
	Handle = FActiveLuxActionHandle::GenerateNewHandle();
}

bool FActiveLuxAction::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

    Ar << Action;
    Ar << Handle.Handle; 

    Spec.NetSerialize(Ar, Map, bOutSuccess);
	if (!bOutSuccess) return false;

	Ar << PredictionKey.Key;

    ActorInfo.NetSerialize(Ar, Map, bOutSuccess);
	if (!bOutSuccess) return false;

	Ar << StartTime;
	Ar.SerializeBits(&bIsDone, 1);

    return bOutSuccess;
}


void FActiveLuxAction::PreReplicatedRemove(const struct FActiveLuxActionContainer& InArraySerializer)
{
	// UE_LOG(LogTemp, Log, TEXT("FActiveLuxAction::PreReplicatedRemove"));
}

void FActiveLuxAction::PostReplicatedAdd(const struct FActiveLuxActionContainer& InArraySerializer)
{
	
}

void FActiveLuxAction::PostReplicatedChange(const struct FActiveLuxActionContainer& InArraySerializer)
{
	// UE_LOG(LogTemp, Log, TEXT("FActiveLuxAction::PostReplicatedChange"));
}

bool FActiveLuxAction::operator==(const FActiveLuxAction& Other) const
{
	return Handle == Other.Handle
		&& Action == Other.Action;
}

bool FActiveLuxAction::operator!=(const FActiveLuxAction& Other) const
{
	return Handle != Other.Handle;
}

void FActiveLuxActionContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid())
	{
		return;
	}

	for (const int32 Index : AddedIndices)
	{
		if (!Items.IsValidIndex(Index))
		{
			continue;
		}

		FActiveLuxAction& AddedAction = Items[Index];
		AActor* AvatarActor = OwnerComponent->GetAvatarActor();
		if (!AvatarActor)
		{
			continue;
		}

		if (AddedAction.Action == nullptr)
		{
			UE_LOG(LogLuxActionSystem, Error, TEXT("Action 포인터가 null인 액션을 수신했습니다."));
			continue;
		}

		if (AvatarActor->GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
		{
			UE_LOG(LogLuxActionSystem, Log, TEXT("[클라이언트] 서버로부터 예측 키 [%d]를 가진 액션 '%s'를 수신하였습니다. 소유권 이전을 시작합니다."),
				AddedAction.PredictionKey.Key, *GetNameSafe(AddedAction.Action));

			OwnerComponent->ReHomePredictedActionTasks(AddedAction);
		}
		else if (AvatarActor->GetLocalRole() == ENetRole::ROLE_SimulatedProxy)
		{
			UE_LOG(LogLuxActionSystem, Log, TEXT("[시뮬레이션 프록시] 복제된 액션 '%s'를 실행합니다."), *GetNameSafe(AddedAction.Action));
			AddedAction.Action->ExecuteAction(AddedAction.ActorInfo, AddedAction.Spec, AddedAction.Handle);
		}
	}
}

void FActiveLuxActionContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid())
	{
		return;
	}

	for (const int32 Index : ChangedIndices)
	{
		if (Items.IsValidIndex(Index))
		{
			FActiveLuxAction& ChangedAction = Items[Index];
			OwnerComponent->ReHomePredictedActionTasks(ChangedAction);
		}
	}
}

void FActiveLuxActionContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	// 활성 액션이 제거되기 직전에 클라이언트에서 호출됩니다.
	if (OwnerComponent.IsValid() == false)
	{
		return;
	}

	for (const int32 Index : RemovedIndices)
	{
		const FActiveLuxAction& RemovedAction = Items[Index];
		if (ULuxAction* Action = RemovedAction.Action)
		{
			//OwnerComponent->ActiveActionMap.Remove(RemovedAction.Handle);
			UE_LOG(LogLuxActionSystem, Log, TEXT("클라이언트: 활성 액션 '%s'가 제거됩니다."), *Action->GetName());
		}
	}
}