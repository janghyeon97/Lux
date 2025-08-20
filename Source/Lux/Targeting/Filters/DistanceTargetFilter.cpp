// Copyright Epic Games, Inc. All Rights Reserved.

#include "DistanceTargetFilter.h"
#include "LuxLogChannels.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UDistanceTargetFilter::UDistanceTargetFilter()
{
	MinDistance = 0.0f;
	MaxDistance = 1000.0f;
	bUseOwnerLocation = true;
	bUse2DDistance = false;
	bDebugLog = false;
}

bool UDistanceTargetFilter::IsTargetValid(AActor* Target, AActor* SourceActor) const
{
	if (::IsValid(Target) == false)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[DistanceTargetFilter] Target is null"));
		}
		return false;
	}

	if (::IsValid(SourceActor) == false)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[DistanceTargetFilter] SourceActor is null"));
		}
		return false;
	}

	// 기준점 결정
	FVector ReferenceLocation;
	if (bUseOwnerLocation)
	{
		ReferenceLocation = SourceActor->GetActorLocation();
	}
	else
	{
		ReferenceLocation = GetCameraLocation(SourceActor);
		// 카메라 위치를 가져올 수 없으면 소유자 위치 사용
		if (ReferenceLocation.IsZero())
		{
			ReferenceLocation = SourceActor->GetActorLocation();
		}
	}

	// 타겟 위치
	FVector TargetLocation = Target->GetActorLocation();

	// 거리 계산
	float Distance;
	if (bUse2DDistance)
	{
		// 2D 거리 (Z축 무시)
		FVector2D ReferenceLocation2D(ReferenceLocation.X, ReferenceLocation.Y);
		FVector2D TargetLocation2D(TargetLocation.X, TargetLocation.Y);
		Distance = FVector2D::Distance(ReferenceLocation2D, TargetLocation2D);
	}
	else
	{
		// 3D 거리
		Distance = FVector::Distance(ReferenceLocation, TargetLocation);
	}

	// 거리 범위 확인
	bool bInRange = true;

	// 최소 거리 확인 (0 이하면 제한 없음)
	if (MinDistance > 0.0f && Distance < MinDistance)
	{
		bInRange = false;
	}

	// 최대 거리 확인 (0 이하면 제한 없음)
	if (MaxDistance > 0.0f && Distance > MaxDistance)
	{
		bInRange = false;
	}

	if (bDebugLog)
	{
		UE_LOG(LogLux, Log, TEXT("[DistanceTargetFilter] %s -> %s: Distance=%.1f, Range=[%.1f, %.1f], InRange=%s"), 
			*SourceActor->GetName(), *Target->GetName(), Distance, MinDistance, MaxDistance, 
			bInRange ? TEXT("Yes") : TEXT("No"));
	}

	return bInRange;
}

FString UDistanceTargetFilter::GetFilterDescription() const
{
	FString Description = TEXT("Distance: ");
	
	if (MinDistance > 0.0f && MaxDistance > 0.0f)
	{
		Description += FString::Printf(TEXT("%.0f - %.0f"), MinDistance, MaxDistance);
	}
	else if (MinDistance > 0.0f)
	{
		Description += FString::Printf(TEXT(">= %.0f"), MinDistance);
	}
	else if (MaxDistance > 0.0f)
	{
		Description += FString::Printf(TEXT("<= %.0f"), MaxDistance);
	}
	else
	{
		Description += TEXT("No limit");
	}
	
	if (bUse2DDistance)
	{
		Description += TEXT(" (2D)");
	}
	
	if (!bUseOwnerLocation)
	{
		Description += TEXT(" (from camera)");
	}
	
	return Description;
}

FVector UDistanceTargetFilter::GetCameraLocation(AActor* Actor) const
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}

	// Pawn인 경우 PlayerController를 통해 카메라 위치 가져오기
	if (APawn* OwnerPawn = Cast<APawn>(Actor))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				return PC->PlayerCameraManager->GetCameraLocation();
			}
		}
	}

	// PlayerController 자체인 경우
	if (APlayerController* PC = Cast<APlayerController>(Actor))
	{
		if (PC->PlayerCameraManager)
		{
			return PC->PlayerCameraManager->GetCameraLocation();
		}
	}

	// 카메라 위치를 가져올 수 없으면 ZeroVector 반환
	return FVector::ZeroVector;
}
