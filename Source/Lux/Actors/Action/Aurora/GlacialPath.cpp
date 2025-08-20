// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Action/Aurora/GlacialPath.h"
#include "ActionSystem/Actions/Aurora/AuroraAction_GlacialCharge.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "LuxLogChannels.h"

AGlacialPath::AGlacialPath()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 시작 시에는 비활성화

	bReplicates = true;
	SetReplicateMovement(true);

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SplineMeshFinder(TEXT("/Game/ParagonAurora/FX/Meshes/Aurora/SM_Aurora_Sculpture_Base.SM_Aurora_Sculpture_Base"));
	if (SplineMeshFinder.Succeeded())
	{
		SplineStaticMesh = SplineMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SplineMaterialFinder(TEXT("/Game/ParagonAurora/FX/Materials/Aurora/FX/M_Aurora_Sculpture_Statue.M_Aurora_Sculpture_Statue"));
	if (SplineMaterialFinder.Succeeded())
	{
		SplineMaterial = SplineMaterialFinder.Object;
	}
}


void AGlacialPath::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (USplineMeshComponent* SplineMesh : SplineMeshComponents)
	{
		if (SplineMesh && SplineMesh->IsValidLowLevel())
		{
			SplineMesh->DestroyComponent();
		}
	}

	SplineMeshComponents.Empty();
}

void AGlacialPath::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSplineMesh(DeltaTime);
}

void AGlacialPath::Initialize(ULuxAction* Action, bool bAutoStart)
{
	Super::Initialize(Action, bAutoStart);
	
	if (!Action)
	{
		UE_LOG(LogLux, Error, TEXT("GlacialPath: Initialize 실패 - Action이 유효하지 않습니다."));
		return;
	}

	if (!SourceASC.IsValid())
	{
		UE_LOG(LogLux, Error, TEXT("GlacialPath: Initialize 실패 - ASC가 유효하지 않습니다."));
		return;
	}

	const FName RowName = FName(*FString::FromInt(ActionLevel));
	const FLuxActionLevelData* LevelData = Action->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("GlacialPath Initialize"));

	if (!LevelData)
	{
		UE_LOG(LogLux, Error, TEXT("GlacialPath: LevelDataTable에서 레벨 %d 데이터를 찾을 수 없습니다."), ActionLevel);
		return;
	}

	const FPayload_PathData* PathData = Action->ActionPayload->GetData<FPayload_PathData>(FName("PathData"));
	if (!PathData || PathData->PathPoints.Num() < 2)
	{
		UE_LOG(LogLux, Error, TEXT("GlacialPath: Initialize 실패 - ActionPayload에서 유효한 경로 데이터를 찾을 수 없습니다."));
		return;
	}

	// 구체적인 초기화 로직
	if(const FAuroraActionLevelData_GlacialCharge* Data = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_GlacialCharge>())
	{
		Multicast_BuildSpline(*Data, PathData->PathPoints, bAutoStart);
	}
}

void AGlacialPath::Multicast_BuildSpline_Implementation(const FAuroraActionLevelData_GlacialCharge& Data, const TArray<FVector>& PathPoints, bool bAutoStart)
{
	if (PathPoints.Num() < 2)
	{
		return;
	}

	LifeTime = Data.PathLifetime > 0.0f ? Data.PathLifetime : 7.0f;
	SplineStaticMesh = Data.SplineMesh.LoadSynchronous();
	SplineMaterial = Data.SplineMaterial.LoadSynchronous();

	if (!SplineStaticMesh)
	{
		UE_LOG(LogLux, Warning, TEXT("AGlacialPath: SplineStaticMesh 로드에 실패했습니다. 데이터 테이블을 확인하세요."));
	}
	if (!SplineMaterial)
	{
		UE_LOG(LogLux, Warning, TEXT("AGlacialPath: SplineMaterial 로드에 실패했습니다. 데이터 테이블을 확인하세요."));
	}

	// 스플라인 컴포넌트에 경로 지점을 설정합니다.
	SplineComponent->ClearSplinePoints();
	for (const FVector& Point : PathPoints)
	{
		SplineComponent->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
		//DrawDebugSphere(GetWorld(), Point, 10.0f, 12, FColor::Green, false, 5.0f);
	}
	SplineComponent->UpdateSpline();

	SplineLength = SplineComponent->GetSplineLength();
	SegmentLength = Data.SegmentLength > 0.0f ? Data.SegmentLength : 50.f;
	SegmentCount = FMath::Max(1, FMath::CeilToInt(SplineLength / SegmentLength));
	SplineMeshComponents.SetNum(SegmentCount);

	ElapsedTime = 0.0f;
	CreationDuration = Data.CreationDuration > 0.0f ? Data.CreationDuration - 0.5f: 2.0f;

	for (int32 i = 0; i < SegmentCount; ++i)
	{
		FName MeshName = *FString::Printf(TEXT("SplineMesh_%d"), i);
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), MeshName);
		if (::IsValid(SplineMesh) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] Failed to create SplineMeshComponent at index: %d on %s"), ANSI_TO_TCHAR(__FUNCTION__), i,
				HasAuthority() ? TEXT("Server") : (GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Standalone")));
			continue;
		}

		SplineMesh->SetIsReplicated(true);
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);
		SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMesh->RegisterComponent();

		SplineMesh->SetStaticMesh(SplineStaticMesh);

		if (SplineMaterial)
		{
			SplineMesh->SetMaterial(0, SplineMaterial);
		}

		SplineMesh->SetVisibility(false);
		SplineMesh->SetRelativeScale3D(FVector(1, 2, 1));
		SplineMeshComponents[i] = SplineMesh;
	}

	if (bAutoStart)
	{
		Start();
	}
}

void AGlacialPath::UpdateSplineMesh(float DeltaTime)
{
	if (SplineLength <= 0.f || SegmentLength <= 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}

	// 시간 업데이트
	ElapsedTime += DeltaTime;

	if (ElapsedTime >= CreationDuration)
	{
		ElapsedTime = CreationDuration;
		DistanceAlongSpline = SplineLength;
		SetActorTickEnabled(false);
	}
	else
	{
		const float Alpha = FMath::Clamp(ElapsedTime / CreationDuration, 0.0f, 1.0f);
		DistanceAlongSpline = FMath::Lerp(0.0f, SplineLength, Alpha);
	}

	// 현재 활성화해야 하는 메시 개수 계산
	int32 ActiveMeshCount = FMath::Clamp(FMath::CeilToInt(DistanceAlongSpline / SegmentLength), 0, SegmentCount);

	// 활성화된 메시 개수만큼만 업데이트하여 점진적으로 생성
	for (int32 i = 0; i < ActiveMeshCount; ++i)
	{
		if (!SplineMeshComponents.IsValidIndex(i))
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid Mesh Index: %d. Max: %d"), ANSI_TO_TCHAR(__FUNCTION__), i, SplineMeshComponents.Num());
			continue;
		}

		USplineMeshComponent* SplineMeshComponent = SplineMeshComponents[i];
		if (!::IsValid(SplineMeshComponent))
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] SplineMeshComponent is null at index %d."), ANSI_TO_TCHAR(__FUNCTION__), i);
			continue;
		}

		// 시작 및 끝 위치 설정
		FVector StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(i * SegmentLength, ESplineCoordinateSpace::Local);
		FVector StartTangent = SplineComponent->GetTangentAtDistanceAlongSpline(i * SegmentLength, ESplineCoordinateSpace::Local);
		FVector EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline((i + 1) * SegmentLength, ESplineCoordinateSpace::Local);
		FVector EndTangent = SplineComponent->GetTangentAtDistanceAlongSpline((i + 1) * SegmentLength, ESplineCoordinateSpace::Local);

		// 스플라인 메시 업데이트
		SplineMeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);

		// 메시 활성화 (처음 활성화될 때만)
		if (!SplineMeshComponent->IsVisible())
		{
			SplineMeshComponent->SetCollisionObjectType(ECC_WorldStatic);
			SplineMeshComponent->SetCollisionProfileName("BlockAllDynamic");
			SplineMeshComponent->SetVisibility(true);
			SplineMeshComponent->UpdateOverlaps();
		}
	}
}