// Copyright Epic Games, Inc. All Rights Reserved.

#include "VisibilityTargetFilter.h"
#include "LuxLogChannels.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"

UVisibilityTargetFilter::UVisibilityTargetFilter()
{
	VisibilityTraceChannel = ECC_Visibility;
	bUseOwnerLocation = false;
	bUseTargetCenter = true;
	TargetOffset = FVector(0, 0, 50);
	bUseComplexCollision = false;
	bDrawDebugLine = false;
	bDebugLog = false;
}

bool UVisibilityTargetFilter::IsTargetValid(AActor* Target, AActor* SourceActor) const
{
	if (::IsValid(Target) == false)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[VisibilityTargetFilter] Target is null"));
		}
		return false;
	}

	if (::IsValid(SourceActor) == false)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[VisibilityTargetFilter] SourceActor is null"));
		}
		return false;
	}

	UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[VisibilityTargetFilter] World is null"));
		}
		return false;
	}

	// 트레이스 시작점과 끝점 계산
	FVector TraceStart = GetTraceStartLocation(SourceActor);
	FVector TraceEnd = bUseTargetCenter ? Target->GetActorLocation() : Target->GetRootComponent()->GetComponentLocation();
	TraceEnd += TargetOffset;

	// 트레이스 설정
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = bUseComplexCollision;
	QueryParams.AddIgnoredActor(SourceActor);
	QueryParams.AddIgnoredActor(Target);
	
	// 추가로 무시할 액터들 등록
	for (AActor* IgnoredActor : IgnoredActors)
	{
		if (IgnoredActor)
		{
			QueryParams.AddIgnoredActor(IgnoredActor);
		}
	}

	// 라인트레이스 수행
	FHitResult HitResult;
	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		VisibilityTraceChannel,
		QueryParams
	);

	// 히트하지 않았다면 타겟이 보임
	bool bIsVisible = !bHit;

	// 디버그 라인 그리기
	if (bDrawDebugLine)
	{
		FColor LineColor = bIsVisible ? FColor::Green : FColor::Red;
		::DrawDebugLine(World, TraceStart, TraceEnd, LineColor, false, 0.1f, 0, 2.0f);
		
		if (bHit)
		{
			// 충돌 지점에 구체 표시
			::DrawDebugSphere(World, HitResult.Location, 10.0f, 8, FColor::Red, false, 0.1f);
		}
	}

	if (bDebugLog)
	{
		if (bHit)
		{
			UE_LOG(LogLux, Log, TEXT("[VisibilityTargetFilter] %s -> %s: BLOCKED by %s"), 
				*SourceActor->GetName(), *Target->GetName(), 
				HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("Unknown"));
		}
		else
		{
			UE_LOG(LogLux, Log, TEXT("[VisibilityTargetFilter] %s -> %s: VISIBLE"), 
				*SourceActor->GetName(), *Target->GetName());
		}
	}

	return bIsVisible;
}

FString UVisibilityTargetFilter::GetFilterDescription() const
{
	FString Description = TEXT("Visibility: ");
	Description += bUseOwnerLocation ? TEXT("from owner") : TEXT("from camera");
	Description += bUseTargetCenter ? TEXT(" to center") : TEXT(" to root");
	
	if (!TargetOffset.IsZero())
	{
		Description += FString::Printf(TEXT(" +offset(%.0f,%.0f,%.0f)"), 
			TargetOffset.X, TargetOffset.Y, TargetOffset.Z);
	}
	
	return Description;
}

FVector UVisibilityTargetFilter::GetCameraLocation(AActor* Actor) const
{
	if (::IsValid(Actor) == false)
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

FVector UVisibilityTargetFilter::GetTraceStartLocation(AActor* Actor) const
{
	if (bUseOwnerLocation)
	{
		return Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
	}
	else
	{
		FVector CameraLocation = GetCameraLocation(Actor);
		if (!CameraLocation.IsZero())
		{
			return CameraLocation;
		}
		
		// 카메라 위치를 가져올 수 없으면 소유자 위치 사용
		return Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
	}
}
