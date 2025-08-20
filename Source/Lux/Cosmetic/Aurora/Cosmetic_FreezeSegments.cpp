// Fill out your copyright notice in the Description page of Project Settings.


#include "Cosmetic/Aurora/Cosmetic_FreezeSegments.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"


ACosmetic_FreezeSegments::ACosmetic_FreezeSegments()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
}

void ACosmetic_FreezeSegments::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACosmetic_FreezeSegments::BeginEffect(UParticleSystem* Particle, float InRadius, int32 InNumSegments, float InSpawnRate)
{
    if (!Particle)
    {
        Destroy();
        return;
    }

    // 파라미터 저장
    ParticleSystem = Particle;
    Radius = InRadius;
    NumSegments = InNumSegments;
    SpawnRate = InSpawnRate;

    // 타이머 시작
    GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ACosmetic_FreezeSegments::SpawnSegment, SpawnRate, true);
}

void ACosmetic_FreezeSegments::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    Super::EndPlay(EndPlayReason);
}

void ACosmetic_FreezeSegments::SpawnSegment()
{
    if (CurrentSegmentIndex >= NumSegments)
    {
        // 모든 파티클 생성 후 액터 스스로 파괴
        Destroy();
        return;
    }

    // 위치 계산
    const float AngleIncrement = 360.0f / NumSegments;
    const float CurrentAngle = CurrentSegmentIndex * AngleIncrement;
    FVector SpawnLocation = GetActorLocation() + GetActorForwardVector().RotateAngleAxis(CurrentAngle, FVector::UpVector) * Radius;
    FRotator SpawnRotation = GetActorRotation();
    SpawnRotation.Yaw += CurrentAngle + 90.f;

    // 파티클 생성
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleSystem, SpawnLocation, SpawnRotation, true);

    CurrentSegmentIndex++;
}