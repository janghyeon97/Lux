// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Cues/LuxCueNotify.h"
#include "CueNotify_Aurora_Hoarfrost.generated.h"


class UParticleSystemComponent;


/**
 * 
 */
UCLASS()
class LUX_API ACueNotify_Aurora_Hoarfrost : public ALuxCueNotify
{
	GENERATED_BODY()
	
public:
	ACueNotify_Aurora_Hoarfrost();

protected:
	//~ UObject Overrides
	virtual void Tick(float DeltaTime) override; 
	//~ End UObject Overrides

	//~ ALuxCueNotify Overrides
	virtual void OnActive_Implementation(AActor* Target, const FLuxCueContext& Context) override;
	virtual void OnStopped_Implementation(AActor* Target, const FLuxCueContext& Context) override;
	//~ End ALuxCueNotify Overrides

private:
	/** 타이머에 의해 주기적으로 호출되어 파티클 세그먼트를 생성합니다. */
	void OnSpawnParticleSegment(); 

	/** 세그먼트 파티클이 재생을 완료했을 때 호출될 콜백 함수입니다. */
	void OnSegmentLifetimeExpired();

	/** 마지막 얼음 세그먼트 파티클의 종료되었을 때 호출됩니다. */
	UFUNCTION()
	void OnLastSegmentFinished(UParticleSystemComponent* ExpiredSystem);

protected:
	/** 얼음 고리 세그먼트의 시각 효과로 사용할 파티클 시스템입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxCue|Assets")
	TObjectPtr<UParticleSystem> SegmentParticle;

	/** 얼음 고리 시작 시각 효과로 사용할 파티클 시스템입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxCue|Assets")
	TObjectPtr<UParticleSystem> WhrilwindParticle;

	/** 얼음 고리 파괴 효과로 사용할 파티클 시스템입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LuxCue|Assets")
	TObjectPtr<UParticleSystem> CrumbleParticle;

	/** 얼음 고리 세그먼트 배열입니다. */
	UPROPERTY()
	TArray<TObjectPtr<UParticleSystemComponent>> SegmentParticleComponents;

private:
	float TimeSinceLastSpawn = 0.0f;

	/** 현재까지 생성된 세그먼트의 개수. */
	int32 CurrentSegmentIndex = 0;
};
