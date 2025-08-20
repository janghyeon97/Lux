// Fill out your copyright notice in the Description page of Project Settings.


#include "Cues/Aurora/CueNotify_Aurora_Hoarfrost.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "LuxLogChannels.h"

ACueNotify_Aurora_Hoarfrost::ACueNotify_Aurora_Hoarfrost()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ACueNotify_Aurora_Hoarfrost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastSpawn += DeltaTime;
	if (TimeSinceLastSpawn >= InitialContext.Rate)
	{
		TimeSinceLastSpawn = 0.0f;
		OnSpawnParticleSegment();
	}
}

void ACueNotify_Aurora_Hoarfrost::OnActive_Implementation(AActor* Target, const FLuxCueContext& Context)
{
	Super::OnActive_Implementation(Target, Context);

	if (InitialContext.Rate <= 0.f)
	{
		return;
	}

	TimeSinceLastSpawn = 0.0f;
	CurrentSegmentIndex = 0;
	SegmentParticleComponents.Empty();

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WhrilwindParticle, InitialContext.Location - FVector(0, 0, 95.f), InitialContext.Rotation, FVector(1), true, EPSCPoolMethod::AutoRelease);

	FTimerHandle NewTimerHandle;
	const float Duration = (InitialContext.Count - 1) * InitialContext.Rate + InitialContext.Lifetime;
	GetWorld()->GetTimerManager().SetTimer(NewTimerHandle, this, &ThisClass::OnSegmentLifetimeExpired, Duration - 1.f, false);
}

void ACueNotify_Aurora_Hoarfrost::OnStopped_Implementation(AActor* Target, const FLuxCueContext& Context)
{
	UE_LOG(LogLuxActionSystem, Warning, TEXT("ACueNotify_Aurora_Hoarfrost::OnStopped_Implementation called. Stopping all particle systems."));

	for (auto& PSC: SegmentParticleComponents)
	{
		if (PSC && PSC->IsActive())
		{
			PSC->OnSystemFinished.RemoveDynamic(this, &ACueNotify_Aurora_Hoarfrost::OnLastSegmentFinished);
			PSC->Deactivate();
		}
	}

	TimeSinceLastSpawn = 0.0f;
	CurrentSegmentIndex = 0;
	SegmentParticleComponents.Empty();

	Super::OnStopped_Implementation(Target, Context);
}

void ACueNotify_Aurora_Hoarfrost::OnSpawnParticleSegment()
{
	// 생성할 파티클 애셋이나 타겟 정보가 없으면 중단합니다.
	if (!SegmentParticle || !CueTarget.IsValid())
	{
		Stop();
		return;
	}

	// 컨텍스트에 저장된 총 개수(Count)만큼 파티클을 모두 생성했다면 타이머를 멈추고 Cue 를 종료합니다..
	if (CurrentSegmentIndex >= InitialContext.Count)
	{
		SetActorTickEnabled(false);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		CurrentSegmentIndex++;
		return;
	}

	// FreezeSegment.cpp의 로직을 참고하여 원형 위치와 회전값을 계산합니다.
	const int32 NumSegments = InitialContext.Count;
	const float Radius = InitialContext.Magnitude;
	const float Angle = 360.f / NumSegments;

	const FVector OriginLocation = InitialContext.Location;
	const FRotator OriginRotation = InitialContext.Rotation;

	const FVector ForwardVector = OriginRotation.Vector();
	const FVector UpVector = OriginRotation.Quaternion().GetUpVector();

	const float CurrentAngleDeg = CurrentSegmentIndex * Angle;
	// 캐릭터 위치를 기준으로 수평 방향의 스폰 위치를 계산합니다.
	const FVector HorizontalSpawnLocation = OriginLocation + ForwardVector.RotateAngleAxis(CurrentAngleDeg, UpVector) * Radius;

	FVector FinalSpawnLocation = HorizontalSpawnLocation;
	FRotator FinalSpawnRotation = ForwardVector.Rotation();
	FinalSpawnRotation.Yaw += CurrentAngleDeg + 90;

	const FVector TraceStart = HorizontalSpawnLocation + FVector(0.f, 0.f, 1000.f);
	const FVector TraceEnd = HorizontalSpawnLocation - FVector(0.f, 0.f, 1000.f);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(CueTarget.Get());
	CollisionParams.AddIgnoredActor(this);

	if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, /*GroundTrace*/ ECC_GameTraceChannel11, CollisionParams))
	{
		FinalSpawnLocation = HitResult.Location;

		// 충돌 지점의 노멀 벡터(표면의 방향)를 사용하여 Z축을 정렬하고, 기존의 Yaw 회전은 유지합니다.
		FinalSpawnRotation = FRotationMatrix::MakeFromZ(HitResult.ImpactNormal).Rotator();
		FinalSpawnRotation.Yaw = OriginRotation.Yaw + CurrentAngleDeg + 90;
	}

	UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(World, SegmentParticle, FinalSpawnLocation, FinalSpawnRotation, FVector(1), true, EPSCPoolMethod::AutoRelease, false);
	if (!PSC)
	{
		CurrentSegmentIndex++;
		return;
	}

	const float Duration = (NumSegments - CurrentSegmentIndex - 1) * InitialContext.Rate + InitialContext.Lifetime;

	PSC->SetFloatParameter(FName("AbilityDuration"), Duration);
	PSC->SetFloatParameter(FName("DurationRange"), Duration);
	PSC->Activate();

	if (CurrentSegmentIndex >= InitialContext.Count -1)
	{
		PSC->OnSystemFinished.AddDynamic(this, &ThisClass::OnLastSegmentFinished);
	}

	SegmentParticleComponents.Add(PSC);
	CurrentSegmentIndex++;
}

void ACueNotify_Aurora_Hoarfrost::OnLastSegmentFinished(UParticleSystemComponent* ExpiredSystem)
{
	Stop();
}

void ACueNotify_Aurora_Hoarfrost::OnSegmentLifetimeExpired()
{
	if (!CrumbleParticle)
	{
		return;
	}

	for (auto& PSC : SegmentParticleComponents)
	{
		if (PSC)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(), 
				CrumbleParticle, 
				PSC->GetComponentLocation(),
				PSC->GetComponentRotation(), 
				FVector(1), 
				true, 
				EPSCPoolMethod::AutoRelease
			);
		}
	}
}