// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cosmetic_FreezeSegments.generated.h"

class UParticleSystem;

UCLASS()
class LUX_API ACosmetic_FreezeSegments : public AActor
{
	GENERATED_BODY()
	
public:	
	ACosmetic_FreezeSegments();

	/** 외부에서 이 이펙트를 시작시키는 함수 */
	void BeginEffect(UParticleSystem* Particle, float InRadius, int32 InNumSegments, float InSpawnRate);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 타이머가 호출할 파티클 생성 함수 */
	void SpawnSegment();

protected:
	// 이펙트 설정값
	UPROPERTY()
	TObjectPtr<UParticleSystem> ParticleSystem;

	float Radius;
	int32 NumSegments;
	float SpawnRate;

	// 내부 상태
	FTimerHandle SpawnTimerHandle;
	int32 CurrentSegmentIndex = 0;
};
