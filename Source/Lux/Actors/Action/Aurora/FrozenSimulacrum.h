// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "Character/LuxAICharacter.h"
#include "Actors/LuxActionSpawnedActorInterface.h"
#include "ActionSystem/Actions/LuxActionTypes.h"
#include "FrozenSimulacrum.generated.h"

class ULuxAction;
class UResourceSet;
class UActionSystemComponent;

struct FInstancedStruct;

UCLASS()
class LUX_API AFrozenSimulacrum : public ALuxAICharacter, public ILuxActionSpawnedActorInterface
{
	GENERATED_BODY()
	
public:	
	AFrozenSimulacrum();

	/** 액션 객체로부터 필요한 정보를 받아 초기화합니다. */
	virtual void Initialize(ULuxAction* Action, bool bAutoStart) override;

protected:
	// ~ AActor interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// ~ End of AActor interface

protected:
	// 액션 정보를 저장하여 CC 효과 추적
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<UActionSystemComponent> SourceASC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lux|Action|Tracking")
	TWeakObjectPtr<ULuxAction> SourceAction = nullptr;

	UPROPERTY()
	FLuxActionSpecHandle ActionSpecHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lux|Action|Tracking")
	FGameplayTag ActionIdentifierTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lux|Action|Tracking")
	int32 ActionLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UResourceSet> ResourceSet;
};
