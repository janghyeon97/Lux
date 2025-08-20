// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuxTeamAgentInterface.h"
#include "LuxTeamStatics.generated.h"

/**
 * 
 */
UCLASS()
class LUX_API ULuxTeamStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** 두 액터 간의 팀 관계(Friendly, Neutral, Hostile)를 반환합니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Teams")
	static ETeamAttitude::Type GetTeamAttitude(const AActor* ActorA, const AActor* ActorB);
};
