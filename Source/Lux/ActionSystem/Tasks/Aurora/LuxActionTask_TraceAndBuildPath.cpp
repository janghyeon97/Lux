// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Tasks/Aurora/LuxActionTask_TraceAndBuildPath.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"


ULuxActionTask_TraceAndBuildPath* ULuxActionTask_TraceAndBuildPath::TraceAndBuildPath(
	ULuxAction* InOwningAction,
	float InTraceDistance,
	float InStepSize,
	float InHeightThreshold,
	float InMaxHeightDifference)
{
	if (!InOwningAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[TraceAndBuildPath] TraceAndBuildPathTask failed on Owning Action is null."));
		return nullptr;
	}

	check(InOwningAction->GetInstancingPolicy() != ELuxActionInstancingPolicy::NonInstanced && 
		"TraceAndBuildPathTask failed: Non-Instanced Actions cannot create Tasks.");

	ULuxActionTask_TraceAndBuildPath* NewTask = NewObject<ULuxActionTask_TraceAndBuildPath>(InOwningAction);
	if (!NewTask)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("TraceAndBuildPathTask failed for Action [%s] on NewTask is null."), *InOwningAction->GetName());
		return nullptr;
	}

	NewTask->TaskName = TEXT("TraceAndBuildPath");
	NewTask->OwningAction = InOwningAction;
	NewTask->TraceDistance = InTraceDistance;
	NewTask->StepSize = InStepSize;
	NewTask->HeightThreshold = InHeightThreshold;
	NewTask->MaxHeightDifference = InMaxHeightDifference;
	NewTask->Activate();

	return NewTask;
}

void ULuxActionTask_TraceAndBuildPath::InitializeFromStruct(const FInstancedStruct& Struct)
{
	if (const FTraceAndBuildPathParams* Params = Struct.GetPtr<FTraceAndBuildPathParams>())
	{
		TraceDistance = Params->TraceDistance.GetValue(OwningAction.Get());
		StepSize = Params->StepSize.GetValue(OwningAction.Get());
		HeightThreshold = Params->HeightThreshold.GetValue(OwningAction.Get());
		MaxHeightDifference = Params->MaxHeightDifference.GetValue(OwningAction.Get());
	}
}


void ULuxActionTask_TraceAndBuildPath::OnActivated()
{
	Super::OnActivated();

	AActor* Avatar = OwningAction.IsValid() ? OwningAction->GetAvatarActor() : nullptr;
	ACharacter* AvatarCharacter = Cast<ACharacter>(Avatar);
	if (!AvatarCharacter || !GetWorld())
	{
		EndTask(false);
		return;
	}

	AController* Controller = AvatarCharacter->GetController();
	if (!Controller)
	{
		EndTask(false);
		return;
	}

	FVector TraceStartLocation = AvatarCharacter->GetActorLocation();
	const FVector TraceDirection = Controller->GetControlRotation().Vector();

	// 캡슐의 반지름만큼 캐릭터의 전방 위치를 시작점으로 설정합니다.
	UCapsuleComponent* Capsule = AvatarCharacter->GetCapsuleComponent();
	if (Capsule)
	{
		const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
		const float CapsuleHeight = Capsule->GetScaledCapsuleHalfHeight();
		TraceStartLocation = AvatarCharacter->GetActorLocation() + (AvatarCharacter->GetActorForwardVector() * CapsuleRadius) - FVector(0, 0, CapsuleHeight);
	}

	// 경로 생성 로직
	TArray<FVector> PathPoints = TraceTerrainPath(TraceStartLocation, AvatarCharacter->GetActorForwardVector());
	if (PathPoints.Num() > 1)
	{
		TArray<FTerrainSegment> Segments = AnalyzeTerrainSegments(PathPoints);
		ProcessTerrainSegments(Segments);
		SmoothTerrainSegments(PathPoints, Segments);
	}

	// 결과 데이터를 페이로드에 담아 액션으로 전송
	FPayload_PathData PathDataPayload;
	PathDataPayload.PathPoints = PathPoints;

	FContextPayload ContextPayload;
	ContextPayload.SetData<FPayload_PathData>(LuxPayloadKeys::PathData, PathDataPayload);

	if (OwningAction.IsValid())
	{
		if (PathPoints.Num() >= 2) // 성공 조건
		{
			OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Path_Ready, ContextPayload);
		}
		else 
		{
			OwningAction->PostTaskEvent(LuxGameplayTags::Task_Event_Path_Failed, ContextPayload);
		}
	}

	EndTask(true);
}

TArray<FVector> ULuxActionTask_TraceAndBuildPath::TraceTerrainPath(const FVector& StartLocation, const FVector& ForwardDirection)
{
	TArray<FVector> OutPath;
	UWorld* World = GetWorld();
	AActor* Avatar = OwningAction.IsValid() ? OwningAction->GetAvatarActor() : nullptr;

	if (!World || !Avatar || TraceDistance <= 0.f || StepSize <= 0.f)
	{
		return OutPath;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	OutPath.Add(StartLocation);

	const int32 Iterations = FMath::CeilToInt(TraceDistance / StepSize);
	for (int32 i = 1; i <= Iterations; ++i)
	{
		const FVector TraceStart = StartLocation + (ForwardDirection * StepSize * i);
		const FVector HighPoint = FVector(TraceStart.X, TraceStart.Y, TraceStart.Z + 5000.f);
		const FVector LowPoint = FVector(TraceStart.X, TraceStart.Y, TraceStart.Z - 5000.f);

		if (World->LineTraceSingleByChannel(HitResult, HighPoint, LowPoint, ECC_WorldStatic, QueryParams))
		{
			if (FMath::Abs(HitResult.Location.Z - OutPath.Last().Z) > HeightThreshold)
			{
				break;
			}
			OutPath.Add(HitResult.Location);
			//DrawDebugLine(GetWorld(), HitResult.Location, HitResult.Location + FVector(0, 0, 100), FColor::Red, false, 5.0f, 0, 1.0f);
		}
		else
		{
			break;
		}
	}
	return OutPath;
}

TArray<FTerrainSegment> ULuxActionTask_TraceAndBuildPath::AnalyzeTerrainSegments(const TArray<FVector>& PathPoints)
{
	TArray<FTerrainSegment> OutTerrainSegments;
	if (PathPoints.Num() < 2)
	{
		return OutTerrainSegments;
	}

	int32 StartIndex = 0;
	FString CurrentType = "Flat";

	for (int32 i = 0; i < PathPoints.Num() - 1; i++)
	{
		const float DeltaZ = PathPoints[i + 1].Z - PathPoints[i].Z;
		FString NewType = FMath::Abs(DeltaZ) <= MaxHeightDifference ? "Flat" : (DeltaZ > 0 ? "Uphill" : "Downhill");

		if (NewType != CurrentType)
		{
			if (NewType == "Downhill")
			{
				OutTerrainSegments.Add(FTerrainSegment(StartIndex, i, CurrentType));
				StartIndex = i;
			}
			else if (NewType == "Uphill")
			{
				OutTerrainSegments.Add(FTerrainSegment(StartIndex, i + 1, CurrentType));
				StartIndex = i;
			}
			else
			{
				OutTerrainSegments.Add(FTerrainSegment(StartIndex, i, CurrentType));
				StartIndex = i + 1;
			}

			CurrentType = NewType;
		}
	}

	OutTerrainSegments.Add(FTerrainSegment(StartIndex, PathPoints.Num() - 1, CurrentType));
	return OutTerrainSegments;
}

void ULuxActionTask_TraceAndBuildPath::ProcessTerrainSegments(TArray<FTerrainSegment>& TerrainSegments)
{
	// Dip 처리 (Downhill -> Flat -> Uphill)
	for (int32 i = 0; i < TerrainSegments.Num() - 1; ++i)
	{
		if (TerrainSegments[i].Type == "Downhill")
		{
			int32 DipStart = i;
			int32 DipEnd = i;

			while (DipEnd + 1 < TerrainSegments.Num() && TerrainSegments[DipEnd + 1].Type == "Flat")
			{
				DipEnd++;
			}

			if (DipEnd + 1 < TerrainSegments.Num() && TerrainSegments[DipEnd + 1].Type == "Uphill")
			{
				TerrainSegments[DipStart].EndIndex = TerrainSegments[DipEnd + 1].EndIndex;
				TerrainSegments[DipStart].Type = "Dip";

				TerrainSegments.RemoveAt(DipStart + 1, DipEnd - DipStart + 1);
				--i;
				continue;
			}
		}
	}

	// 오르막 확장: 이전 Flat 포함
	for (int32 i = 0; i < TerrainSegments.Num(); ++i)
	{
		if (TerrainSegments[i].Type == "Uphill" && i > 0 && TerrainSegments[i - 1].Type == "Flat")
		{
			TerrainSegments[i].StartIndex = TerrainSegments[i - 1].StartIndex;
			TerrainSegments.RemoveAt(i - 1);
			--i;
		}
	}

	// 내리막 확장: 이후 Flat 포함
	for (int32 i = 0; i < TerrainSegments.Num() - 1; ++i)
	{
		if (TerrainSegments[i].Type == "Downhill" && TerrainSegments[i + 1].Type == "Flat")
		{
			TerrainSegments[i].EndIndex = TerrainSegments[i + 1].EndIndex;
			TerrainSegments.RemoveAt(i + 1);
			--i;
		}
	}
}

void ULuxActionTask_TraceAndBuildPath::SmoothTerrainSegments(TArray<FVector>& PathPoints, const TArray<FTerrainSegment>& TerrainSegments)
{
	for (const FTerrainSegment& Segment : TerrainSegments)
	{
		const int32 StartIndex = Segment.StartIndex;
		const int32 EndIndex = Segment.EndIndex;

		if (EndIndex <= StartIndex) continue;

		const FVector StartPoint = PathPoints[StartIndex];
		const FVector EndPoint = PathPoints[EndIndex];

		for (int32 i = StartIndex + 1; i < EndIndex; ++i)
		{
			float Alpha = static_cast<float>(i - StartIndex) / static_cast<float>(EndIndex - StartIndex);

			float EaseAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 1.5f);  //  Ease-In/Ease-Out
			// float EaseAlpha = FMath::InterpCircularInOut(0.0f, 1.0f, Alpha); //  CircularInOut
			// float EaseAlpha = FMath::InterpCubicInOut(0.0f, 1.0f, Alpha);    //  Cubic 보간
			// float EaseAlpha = FMath::InterpSinInOut(0.0f, 1.0f, Alpha);      //  Sine 곡선 보간
			// float EaseAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);          //  SmoothStep 보간
			PathPoints[i].Z = FMath::Lerp(StartPoint.Z, EndPoint.Z, EaseAlpha);
		}
	}
}