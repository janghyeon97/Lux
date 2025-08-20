// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TestPlayerState.generated.h"

class APlayerState;
class ULuxPawnData;
class UActionSystemComponent;

/**
 * 
 */
UCLASS()
class LUX_API ATestPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ATestPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UActionSystemComponent* GetActionSystemComponent() const;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }
	void SetPawnData(const ULuxPawnData* InPawnData);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UActionSystemComponent* ActionSystemComponent;

protected:
	UFUNCTION()
	void OnRep_PawnData();

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const ULuxPawnData> PawnData;
};
