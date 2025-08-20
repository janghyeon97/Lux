// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/LuxGameInstance.h"

#include "Components/GameFrameworkComponentManager.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "Character/LuxPawnData.h"

ULuxGameInstance::ULuxGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PawnDataTable = nullptr;
}

void ULuxGameInstance::Init()
{
	Super::Init();

	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);
	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(LuxGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(LuxGameplayTags::InitState_DataAvailable, false, LuxGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(LuxGameplayTags::InitState_DataInitialized, false, LuxGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(LuxGameplayTags::InitState_GameplayReady, false, LuxGameplayTags::InitState_DataInitialized);
	}
}

void ULuxGameInstance::Shutdown()
{
	Super::Shutdown();
}

/*
UDataTable* ULuxGameInstance::GetPawnDataTable() const
{
	return PawnDataTable;
}

const ULuxPawnData* ULuxGameInstance::FindPawnDataRow(FName RowName) const
{
	// DataTable�� ��ȿ���� Ȯ��
	if (!PawnDataTable)
	{
		UE_LOG(LogLux, Warning, TEXT("FindPawnDataRow: PawnDataTable is not set."));
		return nullptr;
	}

	// FindRow ���ø����� ULuxPawnData ���� �˻�
	static const FString Context = TEXT("FindPawnDataRow");
	const ULuxPawnData* Row = PawnDataTable->FindRow<ULuxPawnData>(RowName, Context);

	// �� ã���� ��� ��� �α�
	if (!Row)
	{
		UE_LOG(LogLux, Warning, TEXT("FindPawnDataRow: Row '%s' not found in PawnDataTable."), *RowName.ToString());
	}

	return Row;
}

bool ULuxGameInstance::IsValidPawnDataRow(FName RowName) const
{
	return PawnDataTable && PawnDataTable->FindRow<ULuxPawnData>(RowName, TEXT(""));
}

TArray<FName> ULuxGameInstance::GetAllPawnDataRowNames() const
{
	return PawnDataTable ? PawnDataTable->GetRowNames() : TArray<FName>();
}

TSubclassOf<APawn> ULuxGameInstance::GetPawnClass(FName RowName) const
{
	if (const ULuxPawnData* Data = FindPawnDataRow(RowName))
		return Data->PawnClass;
	return nullptr;
}
*/