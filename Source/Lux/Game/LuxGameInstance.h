// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "LuxGameInstance.generated.h"

class IEngineLoop;
class UObject;
class UDataTable;
class ULuxPawnData;

USTRUCT(BlueprintType)
struct FCharacterMetaRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCharacterMetaRow()
	{

	}

	// ���� ID (PrimaryAssetId �� Name �κ�)
	UPROPERTY(EditAnywhere) 
	FName PawnDataId;

	// UI�� ������ (Soft Object Pointer)
	UPROPERTY(EditAnywhere) 
	TSoftObjectPtr<UTexture2D> Portrait;

	// UI�� ǥ���� ĳ���� �̸�
	UPROPERTY(EditAnywhere) 
	FText DisplayName;
};


/**
 * 
 */
UCLASS()
class LUX_API ULuxGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	ULuxGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/*UFUNCTION(BlueprintCallable, Category = "Lux|PawnData")
	UDataTable* GetPawnDataTable() const;

	UFUNCTION(BlueprintCallable, Category = "Lux|PawnData")
	const ULuxPawnData* FindPawnDataRow(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "Lux|PawnData")
	bool IsValidPawnDataRow(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "Lux|PawnData")
	TArray<FName> GetAllPawnDataRowNames() const;

	UFUNCTION(BlueprintPure, Category = "Lux|PawnData")
	TSubclassOf<APawn> GetPawnClass(FName RowName) const;*/

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Lux|PawnData")
	UDataTable* PawnDataTable;
};
