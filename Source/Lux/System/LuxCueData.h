// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "LuxCueData.generated.h"

class UDataTable;
class UObject;

/**
 * 게임에서 사용되는 큐(Cue) 관련 데이터들을 관리하는 데이터 에셋입니다.
 * 게임플레이 큐의 정의, 사운드 큐, 비주얼 큐 등과 관련된 데이터를 포함합니다.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Lux Cue Data", ShortTooltip = "Data asset containing cue-related data."))
class LUX_API ULuxCueData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULuxCueData();

	UFUNCTION(BlueprintPure, Category = "Cue Data Access")
	UDataTable* GetCueDataTable() const;

	/*UFUNCTION(BlueprintPure, Category = "Cue Data Access")
	UDataTable* GetSoundCueDataTable() const;

	UFUNCTION(BlueprintPure, Category = "Cue Data Access")
	UDataTable* GetVisualCueDataTable() const;

	UFUNCTION(BlueprintPure, Category = "Cue Data Access")
	UDataTable* GetParticleCueDataTable() const;

	UFUNCTION(BlueprintPure, Category = "Cue Data Access")
	UDataTable* GetAnimationCueDataTable() const;*/

public:
	/** 게임플레이 큐의 정의가 담긴 데이터 테이블입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Cues")
	TSoftObjectPtr<UDataTable> CueDataTable;

	///** 사운드 큐 관련 데이터 테이블입니다. */
	//UPROPERTY(EditDefaultsOnly, Category = "Sound Cues")
	//TSoftObjectPtr<UDataTable> SoundCueDataTable;

	///** 비주얼 큐 관련 데이터 테이블입니다. */
	//UPROPERTY(EditDefaultsOnly, Category = "Visual Cues")
	//TSoftObjectPtr<UDataTable> VisualCueDataTable;

	///** 파티클 큐 관련 데이터 테이블입니다. */
	//UPROPERTY(EditDefaultsOnly, Category = "Particle Cues")
	//TSoftObjectPtr<UDataTable> ParticleCueDataTable;

	///** 애니메이션 큐 관련 데이터 테이블입니다. */
	//UPROPERTY(EditDefaultsOnly, Category = "Animation Cues")
	//TSoftObjectPtr<UDataTable> AnimationCueDataTable;
};
