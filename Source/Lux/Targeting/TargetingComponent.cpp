// Fill out your copyright notice in the Description page of Project Settings.

#include "Targeting/TargetingComponent.h"

// 코어 타겟팅 시스템
#include "Filters/TargetFilter.h"
#include "Teams/LuxTeamStatics.h"
#include "System/LuxAssetManager.h"
#include "System/LuxGameData.h"
#include "LuxTargetingData.h"

// 게임 프레임워크
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// 언리얼 엔진 헤더
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"

#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"

#include "LuxLogChannels.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 기본값들을 먼저 설정 (AssetManager 초기화 전에도 안전하게 동작)
	SetDefaultValues();
	
	// 초기화 시 필요한 변수들
	TargetObjectTypes.Empty();
	PreviousOverlayMaterial = nullptr;
	PreviousMeshComponent = nullptr;
	bWantsInitializeComponent = true;
	
	// 캐시 변수들 초기화
	CachedPlayerController = nullptr;
	CachedPlayerCameraManager = nullptr;
}

void UTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bHasDetectionOverride);
	DOREPLIFETIME(ThisClass, DetectionTypeOverride);
}

void UTargetingComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	// AssetManager가 초기화된 후에 타겟팅 데이터를 로드
	//InitializeTargetingData();
	ClientServerStatus = GetClientServerContextString(this);

}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	ClientServerStatus = GetClientServerContextString(this);
	InitializeTargetingData();
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// AssetManager가 아직 초기화되지 않았고, 기본값을 사용 중이라면 다시 시도
	if (!UAssetManager::IsInitialized())
	{
		return;
	}
	
	// 타겟 업데이트 빈도 제어
	LastTargetUpdateTime += DeltaTime;
	if (LastTargetUpdateTime >= TargetUpdateInterval)
	{
		UpdateTarget();
		LastTargetUpdateTime = 0.0f;
	}
}

void UTargetingComponent::InitializeTargetingData()
{
	// AssetManager가 초기화되었는지 확인
	if (!UAssetManager::IsInitialized())
	{
		UE_LOG(LogLux, Warning, TEXT("[%s][%s] AssetManager not initialized yet, using default values"), *GetName(), *ClientServerStatus);
		SetDefaultValues();
		return;
	}

	// LuxTargetingData에서 타겟팅 관련 설정을 로드합니다.
	ULuxAssetManager& AssetManager = ULuxAssetManager::Get();
	const ULuxGameData& GameData = ULuxGameData::Get();

	const ULuxTargetingData* TargetingData = AssetManager.GetAsset(GameData.TargetingData);
	if (TargetingData)
	{
		// 기본 설정값들을 LuxTargetingData에서 로드
		TraceDistance = TargetingData->TraceDistance;
		OverlapRadius = TargetingData->OverlapRadius;
		SweepRadius = TargetingData->SweepRadius;
		DetectionType = TargetingData->DetectionType;
		TargetObjectTypes = TargetingData->TargetObjectTypes;
		
		// 우선순위 설정
		bEnablePrioritySelection = TargetingData->bEnablePrioritySelection;
		DistanceWeight = TargetingData->DistanceWeight;
		AngleWeight = TargetingData->AngleWeight;
		SizeWeight = TargetingData->SizeWeight;
		
		// 기본 설정
		bEnableTargeting = TargetingData->bEnableTargeting;
		bDrawDebug = TargetingData->bDrawDebug;
		bDrawDebugPriority = TargetingData->bDrawDebugPriority;
		bDrawDebugTrace = TargetingData->bDrawDebugTrace;
		
		// 오버레이 설정
		bEnableOverlayMaterial = TargetingData->bEnableOverlayMaterial;
		HostileOverlayMaterial = TargetingData->HostileOverlayMaterial;
		FriendlyOverlayMaterial = TargetingData->FriendlyOverlayMaterial;
		NeutralOverlayMaterial = TargetingData->NeutralOverlayMaterial;

		// 성능 및 안정성 설정
		TargetUpdateInterval = TargetingData->TargetUpdateInterval;
		TargetChangeThreshold = TargetingData->TargetChangeThreshold;

		UE_LOG(LogLux, Log, TEXT("[%s][%s] Values initialized - TraceDistance: %.1f, OverlapRadius: %.1f, DistanceWeight: %.2f, UpdateInterval: %.2f, ChangeThreshold: %.1f"), 
			*GetName(), *ClientServerStatus, TraceDistance, OverlapRadius, DistanceWeight, TargetUpdateInterval, TargetChangeThreshold);
	}
	else
	{
		UE_LOG(LogLux, Warning, TEXT("[%s][%s] Failed to load TargetingData from GameData, using default values"), *GetName(), *ClientServerStatus);
		UE_LOG(LogLux, Warning, TEXT("[%s][%s] GameData.TargetingData: %s"), *GetName(), *ClientServerStatus, 
			GameData.TargetingData ? *GameData.TargetingData->GetName() : TEXT("nullptr"));
		SetDefaultValues();
	}
}

void UTargetingComponent::SetDefaultValues()
{
	// 기본값 설정
	TraceDistance = 1000.0f;
	OverlapRadius = 2500.0f;
	SweepRadius = 50.0f;
	DetectionType = ETargetingDetectionType::LineTrace;
	bEnablePrioritySelection = true;
	bEnableTargeting = true;
	DistanceWeight = 0.5f;
	AngleWeight = 0.3f;
	SizeWeight = 0.2f;
	bDrawDebugTrace = true;
	bEnableOverlayMaterial = true;
	HostileOverlayMaterial = nullptr;
	FriendlyOverlayMaterial = nullptr;
	NeutralOverlayMaterial = nullptr;

	TargetUpdateInterval = 0.1f;
	TargetChangeThreshold = 50.0f;
}


void UTargetingComponent::UpdateTarget()
{
	// 타겟팅이 비활성화되어 있으면 업데이트하지 않습니다.
	if (!bEnableTargeting)
	{
		return;
	}

	// 항상 새로운 타겟을 검색합니다 (더 나은 타겟이 있을 수 있음)
	AActor* PreviousTarget = CurrentTarget;
	
	// DefaultTargetingFilters에서 유효하지 않은 필터들을 제거
	DefaultTargetingFilters.RemoveAll([](const TObjectPtr<UTargetFilter>& Filter)
	{
		return !::IsValid(Filter);
	});
	
	AActor* NewTarget = FindBestTarget(EAttitudeQuery::Any, DefaultTargetingFilters);
	bool bTargetChanged = false;
	
	// 타겟 포인터가 변경된 경우
	if (PreviousTarget != NewTarget)
	{
		bTargetChanged = true;
	}
	// 현재 타겟이 유효하지 않은 경우 (죽었거나 숨겨진 경우)
	else if (CurrentTarget && (!IsValid(CurrentTarget) || !IsValidTarget(CurrentTarget)))
	{
		bTargetChanged = true;
	}
	
	// 타겟이 변경된 경우에만 처리합니다
	if (bTargetChanged)
	{
		// 이전 타겟의 오버레이 머테리얼을 제거합니다.
		if (PreviousTarget && bEnableOverlayMaterial)
		{
			RemoveOverlayMaterial(PreviousTarget);
		}

		CurrentTarget = NewTarget;
		
		// 새로운 타겟에 오버레이 머테리얼을 적용합니다.
		if (CurrentTarget && bEnableOverlayMaterial)
		{
			ApplyOverlayMaterial(CurrentTarget);
		}
	}
}

void UTargetingComponent::FindOverlapTarget(TArray<AActor*>& OutTargets, EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters)
{
	OutTargets.Empty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 오버랩 검출을 위한 콜리전 쿼리 파라미터를 설정합니다.
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams ObjectQueryParams;
	
	// TargetObjectTypes가 설정되어 있으면 해당 타입들을 사용하고, 설정되어 있지 않으면 기본값으로 ECC_Pawn을 사용합니다.
	if (TargetObjectTypes.Num() > 0)
	{
		for (TEnumAsByte<EObjectTypeQuery> ObjectType : TargetObjectTypes)
		{
			ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
			ObjectQueryParams.AddObjectTypesToQuery(CollisionChannel);
		}
	}
	else
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(OverlapRadius);

	// 오버랩 검출을 수행합니다.
	const bool bHit = World->OverlapMultiByObjectType(
		OverlapResults,
		GetOwner()->GetActorLocation(),
		GetOwner()->GetActorRotation().Quaternion(),
		ObjectQueryParams,
		CollisionShape,
		QueryParams
	);

	if (OverlapResults.Num() == 0)
	{
		return;
	}

	// 검출된 액터들을 필터링합니다.
	int32 ValidCandidates = 0;
	int32 FilteredOutByTeam = 0;
	int32 FilteredOutByAdditionalFilters = 0;
	
	for (FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!IsValid(Candidate) || Candidate == GetOwner())
		{
			continue;
		}
		ValidCandidates++;

		// 팀 관계를 확인합니다.
		ETeamAttitude::Type ActualAttitude = ULuxTeamStatics::GetTeamAttitude(GetOwner(), Candidate);
		if (AttitudeQuery == EAttitudeQuery::Hostile && ActualAttitude != ETeamAttitude::Hostile)
		{
			FilteredOutByTeam++;
			continue;
		}
		else if (AttitudeQuery == EAttitudeQuery::Friendly && ActualAttitude != ETeamAttitude::Friendly)
		{
			FilteredOutByTeam++;
			continue;
		}
		else if (AttitudeQuery == EAttitudeQuery::Neutral && ActualAttitude != ETeamAttitude::Neutral)
		{
			FilteredOutByTeam++;
			continue;
		}

		// 추가 필터를 적용합니다.
		bool bFilterPassed = true;
		for (UTargetFilter* Filter : AdditionalFilters)
		{
			if (::IsValid(Filter) && !Filter->IsTargetValid(Candidate, GetOwner()))
			{
				bFilterPassed = false;
				break;
			}
		}

		if (!bFilterPassed)
		{
			FilteredOutByAdditionalFilters++;
			continue;
		}

		OutTargets.Add(Candidate);
	}
	
	//UE_LOG(LogLux, Warning, TEXT("[%s][%s] Overlap 필터링 결과 - 유효한 후보: %d, 팀 관계로 필터링: %d, 추가 필터로 필터링: %d, 최종 타겟: %d"), 
	//	*GetName(), *ClientServerStatus, ValidCandidates, FilteredOutByTeam, FilteredOutByAdditionalFilters, OutTargets.Num());
}

void UTargetingComponent::FindMultiLineTraceTarget(TArray<AActor*>& OutTargets, EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters)
{
	OutTargets.Empty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// PlayerController를 찾거나 캐시된 값을 사용
	APlayerController* PC = GetOrFindPlayerController();
	if (!PC)
	{
		return;
	}

	// PlayerCameraManager를 찾거나 캐시된 값을 사용
	APlayerCameraManager* CameraManager = GetOrFindPlayerCameraManager();
	if (!CameraManager)
	{
		UE_LOG(LogLux, Warning, TEXT("[%s][%s] PlayerCameraManager not found"), *GetName(), *ClientServerStatus);
		return;
	}

	// 카메라의 현재 뷰포인트를 기준으로 LineTrace를 수행합니다.
	FVector CameraLocation;
	FRotator CameraRotation;
	CameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);

	// TraceDistance만큼 카메라 방향으로 트레이스를 수행합니다.
	FVector TraceEnd = CameraLocation + (CameraRotation.Vector() * TraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams ObjectQueryParams;
	
	// TargetObjectTypes가 설정되어 있으면 해당 타입들을 사용하고 설정되어 있지 않으면 기본값으로 ECC_Pawn을 사용합니다.
	if (TargetObjectTypes.Num() > 0)
	{
		for (TEnumAsByte<EObjectTypeQuery> ObjectType : TargetObjectTypes)
		{
			ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
			ObjectQueryParams.AddObjectTypesToQuery(CollisionChannel);
		}
	}
	else
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	}

	// 카메라에서 TraceDistance만큼 직선으로 트레이스를 수행하여 모든 Pawn을 찾습니다.
	TArray<FHitResult> HitResults;
	const bool bHit = World->LineTraceMultiByObjectType(
		HitResults,
		CameraLocation,
		TraceEnd,
		ObjectQueryParams,
		QueryParams
	);

	if (!bHit || HitResults.Num() == 0)
	{
		return;
	}

	// 모든 히트 결과를 처리하여 유효한 타겟들을 찾습니다.
	int32 ValidCandidates = 0;
	int32 FilteredOutByTeam = 0;
	int32 FilteredOutByAdditionalFilters = 0;
	
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValid(HitActor))
		{
			continue;
		}
		ValidCandidates++;

		// Overlap과 동일한 팀 관계 검사 로직을 적용합니다.
		ETeamAttitude::Type ActualAttitude = ULuxTeamStatics::GetTeamAttitude(GetOwner(), HitActor);
		if (AttitudeQuery == EAttitudeQuery::Hostile && ActualAttitude != ETeamAttitude::Hostile)
		{
			FilteredOutByTeam++;
			continue;
		}
		else if (AttitudeQuery == EAttitudeQuery::Friendly && ActualAttitude != ETeamAttitude::Friendly)
		{
			FilteredOutByTeam++;
			continue;
		}
		else if (AttitudeQuery == EAttitudeQuery::Neutral && ActualAttitude != ETeamAttitude::Neutral)
		{
			FilteredOutByTeam++;
			continue;
		}

		// 추가 필터를 적용합니다. 하나라도 실패하면 해당 타겟은 제외합니다.
		bool bFilterPassed = true;
		for (UTargetFilter* Filter : AdditionalFilters)
		{
			if (::IsValid(Filter) && !Filter->IsTargetValid(HitActor, GetOwner()))
			{
				bFilterPassed = false;
				break;
			}
		}

		if (!bFilterPassed)
		{
			FilteredOutByAdditionalFilters++;
			continue;
		}

		OutTargets.Add(HitActor);
	}
	
	//UE_LOG(LogLux, Warning, TEXT("[%s][%s] LineTrace 필터링 결과 - 유효한 후보: %d, 팀 관계로 필터링: %d, 추가 필터로 필터링: %d, 최종 타겟: %d"), 
	//	*GetName(), *ClientServerStatus, ValidCandidates, FilteredOutByTeam, FilteredOutByAdditionalFilters, OutTargets.Num());
}

void UTargetingComponent::FindSweepTraceTarget(TArray<AActor*>& OutTargets, EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters)
{
	OutTargets.Empty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// PlayerController를 찾거나 캐시된 값을 사용
	APlayerController* PC = GetOrFindPlayerController();
	if (!PC)
	{
		return;
	}

	// PlayerCameraManager를 찾거나 캐시된 값을 사용
	APlayerCameraManager* CameraManager = GetOrFindPlayerCameraManager();
	if (!CameraManager)
	{
		return;
	}

	// 카메라 위치와 방향을 가져옵니다.
	FVector CameraLocation;
	FRotator CameraRotation;
	CameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);

	// TraceDistance만큼 카메라 방향으로 스윕을 수행합니다.
	FVector TraceEnd = CameraLocation + (CameraRotation.Vector() * TraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams ObjectQueryParams;
	
	// TargetObjectTypes가 설정되어 있으면 해당 타입들을 사용하고, 설정되어 있지 않으면 기본값으로 ECC_Pawn을 사용합니다.
	if (TargetObjectTypes.Num() > 0)
	{
		for (TEnumAsByte<EObjectTypeQuery> ObjectType : TargetObjectTypes)
		{
			ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
			ObjectQueryParams.AddObjectTypesToQuery(CollisionChannel);
		}
	}
	else
	{
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	}

	// SweepRadius 크기의 구체로 스윕을 수행합니다.
	FCollisionShape SweepShape = FCollisionShape::MakeSphere(SweepRadius);

	TArray<FHitResult> HitResults;
	const bool bHit = World->SweepMultiByObjectType(
		HitResults,
		CameraLocation,
		TraceEnd,
		CameraManager->GetCameraRotation().Quaternion(),
		ObjectQueryParams,
		SweepShape,
		QueryParams
	);


	if (HitResults.Num() == 0)
	{
		return;
	}

	// 모든 히트 결과를 처리하여 유효한 타겟들을 찾습니다.
	int32 ValidCandidates = 0;
	int32 FilteredOutByTeam = 0;
	int32 FilteredOutByAdditionalFilters = 0;
	
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValid(HitActor))
		{
			continue;
		}
		ValidCandidates++;

		// 팀 관계를 확인합니다.
		ETeamAttitude::Type ActualAttitude = ULuxTeamStatics::GetTeamAttitude(GetOwner(), HitActor);

		if (AttitudeQuery == EAttitudeQuery::Hostile && ActualAttitude != ETeamAttitude::Hostile)
		{
			FilteredOutByTeam++;
			continue;
		}
		else if (AttitudeQuery == EAttitudeQuery::Friendly && ActualAttitude != ETeamAttitude::Friendly)
		{
			FilteredOutByTeam++;
			continue;
		}
		else if (AttitudeQuery == EAttitudeQuery::Neutral && ActualAttitude != ETeamAttitude::Neutral)
		{
			FilteredOutByTeam++;
			continue;
		}

		// 추가 필터를 적용합니다. 하나라도 실패하면 해당 타겟은 제외합니다.
		bool bFilterPassed = true;
		for (UTargetFilter* Filter : AdditionalFilters)
		{
			if (::IsValid(Filter) && !Filter->IsTargetValid(HitActor, GetOwner()))
			{
				bFilterPassed = false;
				break;
			}
		}

		if (!bFilterPassed)
		{
			FilteredOutByAdditionalFilters++;
			continue;
		}

		OutTargets.Add(HitActor);
	}
	
	//UE_LOG(LogLux, Warning, TEXT("[%s][%s] SweepTrace 필터링 결과 - 유효한 후보: %d, 팀 관계로 필터링: %d, 추가 필터로 필터링: %d, 최종 타겟: %d"), 
	//	*GetName(), *ClientServerStatus, ValidCandidates, FilteredOutByTeam, FilteredOutByAdditionalFilters, OutTargets.Num());
}

AActor* UTargetingComponent::FindBestTarget(EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters)
{
	AActor* OutBestTarget = nullptr;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLux, Error, TEXT("[%s][%s] 월드를 찾을 수 없습니다."), *GetName(), *ClientServerStatus);
		return OutBestTarget;
	}

	TArray<AActor*> AllCandidates;
	
    const ETargetingDetectionType ActiveDetectionType = GetActiveDetectionType();
    switch (ActiveDetectionType)
	{
	case ETargetingDetectionType::Overlap:
	{
		// Overlap 방식으로 여러 타겟 검출
		FindOverlapTarget(AllCandidates, AttitudeQuery, AdditionalFilters);
	}
	break;

	case ETargetingDetectionType::LineTrace:
	{
		// LineTrace 방식으로 여러 타겟 검출
		FindMultiLineTraceTarget(AllCandidates, AttitudeQuery, AdditionalFilters);
	}
	break;

	case ETargetingDetectionType::SweepTrace:
	{
		// SweepTrace 방식으로 여러 타겟 검출
		FindSweepTraceTarget(AllCandidates, AttitudeQuery, AdditionalFilters);
	}
	break;

	default:
	{
		// 기본값으로 Overlap 사용
		UE_LOG(LogLux, Warning, TEXT("[%s][%s] 알 수 없는 DetectionType(%d), Overlap 방식으로 검출"), *GetName(), *ClientServerStatus, (int32)ActiveDetectionType);
		FindOverlapTarget(AllCandidates, AttitudeQuery, AdditionalFilters);
	}
	break;
	}

	// 검출된 타겟이 없으면 종료
	if (AllCandidates.Num() == 0)
	{
		return OutBestTarget;
	}

	// 검출된 타겟이 하나뿐이면 해당 타겟 반환
	if (AllCandidates.Num() == 1)
	{
		OutBestTarget = AllCandidates[0];
		return OutBestTarget;
	}

	// 우선순위 선택이 활성화되어 있으면 정렬을 수행합니다.
	if (bEnablePrioritySelection)
	{
		AllCandidates.Sort([this](const AActor& A, const AActor& B)
		{
			TArray<AActor*> TempArray = {const_cast<AActor*>(&A), const_cast<AActor*>(&B)};
			return SelectBestTargetByPriority(TempArray) == &A;
		});
	}

	// 정렬된 후보들 중에서 최고 우선순위 타겟을 선택합니다.
	OutBestTarget = SelectBestTargetByPriority(AllCandidates);
	return OutBestTarget;
}

AActor* UTargetingComponent::SelectBestTargetByPriority(const TArray<AActor*>& Candidates)
{
	// 후보가 없으면 nullptr을 반환합니다.
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	// 후보가 하나뿐이면 해당 후보를 반환합니다.
	if (Candidates.Num() == 1)
	{
		return Candidates[0];
	}

	// 우선순위 선택 알고리즘:
	// 1. 거리 점수: 가까울수록 높은 점수 (Exp(-Distance * DecayRate) * DistanceWeight)
	// 2. 각도 점수: 화면 중앙에 가까울수록 높은 점수 ((DotProduct + 1) * 0.5 * AngleWeight)
	// 3. 크기 점수: 액터의 실제 콜리전 크기가 클수록 높은 점수 (NormalizedSize * SizeWeight)
	// 
	// 각 가중치는 DistanceWeight, AngleWeight, SizeWeight로 조정 가능합니다.
	// 총 점수 = 거리점수 + 각도점수 + 크기점수
	//
	// 가중치가 0인 경우:
	// - DistanceWeight가 0이면 거리 계산을 건너뛰고 DistanceScore = 0
	// - AngleWeight가 0이면 각도 계산을 건너뛰고 AngleScore = 0  
	// - SizeWeight가 0이면 크기 계산을 건너뛰고 SizeScore = 0
	// - 모든 가중치가 0이면 모든 점수가 0이 되어 첫 번째 후보가 선택됨
	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (AActor* Candidate : Candidates)
	{
		if (!::IsValid(Candidate))
		{
			continue;
		}

		// 가까울수록 높은 점수를 받으며, DistanceWeight로 가중치를 조정할 수 있습니다.
		// 지수 함수를 사용하여 자연스러운 거리 감소를 구현합니다.
		float DistanceScore = 0.0f;
		if (DistanceWeight > 0.0f)
		{
			const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
			
			// 지수 감쇠를 사용하여 거리에 따른 점수 계산
			// DecayRate: 거리 감쇠율 (값이 클수록 거리에 더 민감하게 반응)
			// DistanceWeight: 거리 점수의 전체적인 가중치
			const float DecayRate = 0.001f; // 1000 유닛당 약 37% 감소
			DistanceScore = FMath::Exp(-Distance * DecayRate) * DistanceWeight;
		}

		// 화면 중앙에 가까울수록 높은 점수를 받으며, AngleWeight로 가중치를 조정할 수 있습니다.
		// DotProduct는 -1(반대 방향) ~ 1(같은 방향) 범위를 가지며, 이를 0~1 범위로 정규화합니다.
		float AngleScore = 0.0f;
		if (AngleWeight > 0.0f)
		{
			const FVector DirectionToTarget = (Candidate->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
			const FVector ForwardVector = GetOwner()->GetActorForwardVector();
			const float DotProduct = FVector::DotProduct(ForwardVector, DirectionToTarget);
			AngleScore = (DotProduct + 1.0f) * 0.5f * AngleWeight;
		}

		// 액터의 실제 콜리전 크기가 클수록 높은 점수를 받으며, SizeWeight로 가중치를 조정할 수 있습니다.
		// 크기를 0~1 범위로 정규화하여 일관된 점수를 계산합니다.
		float SizeScore = 0.0f;
		if (SizeWeight > 0.0f)
		{
			// 스케일 대신 실제 콜리전 크기를 사용
			FVector Origin, BoxExtent;
			Candidate->GetActorBounds(false, Origin, BoxExtent);
			
			// 콜리전 박스의 크기를 기준으로 점수 계산
			// BoxExtent는 반지름이므로 실제 크기는 2배
			const float CollisionSize = BoxExtent.Size() * 2.0f;
			
			// 크기를 0~1 범위로 정규화
			// 예상되는 최소/최대 크기 범위를 기준으로 정규화
			const float MinExpectedSize = 50.0f;   // 최소 예상 크기 (예: 작은 캐릭터)
			const float MaxExpectedSize = 500.0f;  // 최대 예상 크기 (예: 큰 보스)
			const float NormalizedSize = FMath::Clamp((CollisionSize - MinExpectedSize) / (MaxExpectedSize - MinExpectedSize), 0.0f, 1.0f);
			
			SizeScore = NormalizedSize * SizeWeight;
		}

		const float TotalScore = DistanceScore + AngleScore + SizeScore;

		// 각 후보의 점수와 세부 점수를 화면에 표시합니다.
		if (bDrawDebugPriority && !Candidate->HasAuthority())
		{
			FString DebugText = FString::Printf(TEXT("Score: %.2f (D:%.2f, A:%.2f, S:%.2f)"),
				TotalScore, DistanceScore, AngleScore, SizeScore);

			::DrawDebugString(GetWorld(), Candidate->GetActorLocation(),
				DebugText, nullptr, FColor::Yellow, TargetUpdateInterval, true, 2.f);
		}

		// 최고 점수를 업데이트합니다.
		if (TotalScore > BestScore)
		{
			BestScore = TotalScore;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool UTargetingComponent::IsValidTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	if (::IsValid(Target) == false)
	{
		return false;
	}

	// 자기 자신을 타겟으로 설정할 수 없습니다.
	if (Target == GetOwner())
	{
		return false;
	}

	// 타겟이 Pawn이 아니면 유효하지 않습니다.
	if (!Cast<APawn>(Target))
	{
		return false;
	}

	// 타겟이 숨겨져 있지 않은지 확인합니다.
	if (Target->IsHidden())
	{
		return false;
	}

	return true;
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	// 이전 타겟의 오버레이 머테리얼을 제거합니다.
	if (::IsValid(CurrentTarget) && bEnableOverlayMaterial)
	{
		RemoveOverlayMaterial(CurrentTarget);
	}

	// 여기에 이전 타겟에 대한 정리 로직을 추가할 수 있습니다.
	// 예: 이전 타겟의 하이라이트 제거, UI 업데이트 등
	if (::IsValid(CurrentTarget))
	{

	}

	CurrentTarget = NewTarget;

	// 새로운 타겟에 오버레이 머테리얼을 적용합니다.
	if (CurrentTarget && bEnableOverlayMaterial)
	{
		ApplyOverlayMaterial(CurrentTarget);
	}

	// 다른 시스템들이 타겟 변경을 감지할 수 있도록 합니다.
	OnTargetChanged.Broadcast(CurrentTarget);
}

void UTargetingComponent::ClearTarget()
{
	if (!CurrentTarget)
	{
		return;
	}

	// 타겟을 정리합니다.
	SetTarget(nullptr);
}

bool UTargetingComponent::HasValidTarget() const
{
	return IsValidTarget(CurrentTarget);
}

float UTargetingComponent::GetDistanceToTarget() const
{
	if (!HasValidTarget())
	{
		return -1.0f;
	}

	// 타겟까지의 거리를 계산합니다.
	return FVector::Dist(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
}

FVector UTargetingComponent::GetDirectionToTarget() const
{
	if (!HasValidTarget())
	{
		return FVector::ZeroVector;
	}

	// 타겟까지의 방향을 계산합니다.
	// GetSafeNormal()을 사용하여 제로 벡터가 되는 경우를 방지합니다.
	return (CurrentTarget->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
}

void UTargetingComponent::SetOverlapRadius(float NewRadius)
{
	if (NewRadius < 0.0f)
	{
		return;
	}

	OverlapRadius = NewRadius;
}

void UTargetingComponent::SetTraceDistance(float NewDistance)
{
	if (NewDistance < 0.0f)
	{
		return;
	}

	TraceDistance = NewDistance;
}

void UTargetingComponent::SetSweepRadius(float NewRadius)
{
	if (NewRadius < 0.0f)
	{
		return;
	}

	SweepRadius = NewRadius;
}

void UTargetingComponent::SetPriorityWeights(float NewDistanceWeight, float NewAngleWeight, float NewSizeWeight)
{
	// 가중치가 음수이면 설정하지 않습니다.
	// 음수 가중치는 우선순위 선택 알고리즘에서 예상치 못한 결과를 초래할 수 있습니다.
	if (NewDistanceWeight < 0.0f || NewAngleWeight < 0.0f || NewSizeWeight < 0.0f)
	{
		return;
	}

	DistanceWeight = NewDistanceWeight;
	AngleWeight = NewAngleWeight;
	SizeWeight = NewSizeWeight;
}

void UTargetingComponent::SetEnableTargeting(bool bEnable)
{
	// 상태가 변경되지 않았으면 아무것도 하지 않습니다.
	// 불필요한 이벤트 발생과 정리 작업을 방지합니다.
	if (bEnableTargeting == bEnable)
	{
		return;
	}

	bEnableTargeting = bEnable;

	// 타겟팅이 비활성화되면 현재 타겟을 정리합니다.
	// 타겟팅이 비활성화된 상태에서 타겟을 유지하는 것은 논리적으로 맞지 않습니다.
	if (!bEnableTargeting)
	{
		ClearTarget();
	}
}

void UTargetingComponent::SetEnablePrioritySelection(bool bEnable)
{
	bEnablePrioritySelection = bEnable;
}

void UTargetingComponent::SetTargetObjectTypes(const TArray<TEnumAsByte<EObjectTypeQuery>>& NewObjectTypes)
{
	// 새로운 오브젝트 타입들을 설정합니다.
	// 이 설정은 다음 검출부터 적용됩니다.
	TargetObjectTypes = NewObjectTypes;
}

void UTargetingComponent::SetEnableOverlayMaterial(bool bEnable)
{
	bEnableOverlayMaterial = bEnable;

	// 오버레이 머테리얼 기능을 비활성화하면 현재 적용된 오버레이를 제거합니다.
	if (!bEnableOverlayMaterial)
	{
		if (PreviousMeshComponent && PreviousOverlayMaterial)
		{
			RemoveOverlayMaterial(CurrentTarget);
		}
	}
}

void UTargetingComponent::SetHostileOverlayMaterial(UMaterialInterface* NewMaterial)
{
	HostileOverlayMaterial = NewMaterial;
	
	// 현재 타겟이 적대적 팀이고 오버레이가 활성화되어 있으면 즉시 적용합니다.
	if (CurrentTarget && bEnableOverlayMaterial)
	{
		ETeamAttitude::Type Attitude = ULuxTeamStatics::GetTeamAttitude(GetOwner(), CurrentTarget);
		if (Attitude == ETeamAttitude::Hostile)
		{
			ApplyOverlayMaterial(CurrentTarget);
		}
	}
}

void UTargetingComponent::SetFriendlyOverlayMaterial(UMaterialInterface* NewMaterial)
{
	FriendlyOverlayMaterial = NewMaterial;
	
	// 현재 타겟이 우호적 팀이고 오버레이가 활성화되어 있으면 즉시 적용합니다.
	if (CurrentTarget && bEnableOverlayMaterial)
	{
		ETeamAttitude::Type Attitude = ULuxTeamStatics::GetTeamAttitude(GetOwner(), CurrentTarget);
		if (Attitude == ETeamAttitude::Friendly)
		{
			ApplyOverlayMaterial(CurrentTarget);
		}
	}
}

void UTargetingComponent::SetNeutralOverlayMaterial(UMaterialInterface* NewMaterial)
{
	NeutralOverlayMaterial = NewMaterial;
	
	// 현재 타겟이 중립 팀이고 오버레이가 활성화되어 있으면 즉시 적용합니다.
	if (CurrentTarget && bEnableOverlayMaterial)
	{
		ETeamAttitude::Type Attitude = ULuxTeamStatics::GetTeamAttitude(GetOwner(), CurrentTarget);
		if (Attitude == ETeamAttitude::Neutral)
		{
			ApplyOverlayMaterial(CurrentTarget);
		}
	}
}

void UTargetingComponent::InvalidateCachedReferences()
{
	// 캐시된 참조들을 무효화하여 다음 호출 시 다시 가져오도록 합니다.
	CachedPlayerController = nullptr;
	CachedPlayerCameraManager = nullptr;
	
	UE_LOG(LogLux, Log, TEXT("[UTargetingComponent::InvalidateCachedReferences] Cached references invalidated"));
}

APlayerController* UTargetingComponent::GetOrFindPlayerController()
{
	// 캐시된 PlayerController가 유효하지 않으면 다시 가져오기
	if (::IsValid(CachedPlayerController) == false)
	{
		CachedPlayerController = Cast<APlayerController>(GetOwner());
		
		if (!CachedPlayerController)
		{
			if (APawn* Pawn = Cast<APawn>(GetOwner()))
			{
				CachedPlayerController = Cast<APlayerController>(Pawn->GetController());
			}
		}
	}
	
	return CachedPlayerController;
}

APlayerCameraManager* UTargetingComponent::GetOrFindPlayerCameraManager()
{
	// PlayerController를 먼저 가져오기
	APlayerController* PC = GetOrFindPlayerController();
	if (!PC)
	{
		return nullptr;
	}
	
	// 캐시된 PlayerCameraManager가 유효하지 않으면 다시 가져오기
	if (!IsValid(CachedPlayerCameraManager))
	{
		CachedPlayerCameraManager = PC->PlayerCameraManager;
	}
	
	return CachedPlayerCameraManager;
}

void UTargetingComponent::ApplyOverlayMaterial(AActor* Target)
{
	if (!Target || !bEnableOverlayMaterial)
	{
		return;
	}

	// 타겟의 메시 컴포넌트를 찾습니다.
	UMeshComponent* MeshComponent = nullptr;
	
	// StaticMeshComponent를 먼저 찾습니다.
	MeshComponent = Target->FindComponentByClass<UStaticMeshComponent>();
	
	// StaticMeshComponent가 없으면 SkeletalMeshComponent를 찾습니다.
	if (!MeshComponent)
	{
		MeshComponent = Target->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (!MeshComponent)
	{
		return;
	}

	// 팀 관계에 따른 적절한 오버레이 머테리얼을 가져옵니다.
	UMaterialInterface* OverlayMaterial = GetOverlayMaterialForTeam(Target);
	if (!OverlayMaterial)
	{
		return;
	}

	// 이전 오버레이 정보를 저장합니다.
	PreviousMeshComponent = MeshComponent;
	PreviousOverlayMaterial = MeshComponent->GetOverlayMaterial();

	// 오버레이 머테리얼을 적용합니다.
	MeshComponent->SetOverlayMaterial(OverlayMaterial);
}

void UTargetingComponent::RemoveOverlayMaterial(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	// 타겟의 메시 컴포넌트를 찾습니다.
	UMeshComponent* MeshComponent = nullptr;
	
	// StaticMeshComponent를 먼저 찾습니다.
	MeshComponent = Target->FindComponentByClass<UStaticMeshComponent>();
	
	// StaticMeshComponent가 없으면 SkeletalMeshComponent를 찾습니다.
	if (!MeshComponent)
	{
		MeshComponent = Target->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (!MeshComponent)
	{
		return;
	}

	// 이전 오버레이 머테리얼이 있으면 복원하고, 없으면 오버레이를 제거합니다.
	if (PreviousOverlayMaterial && PreviousMeshComponent == MeshComponent)
	{
		MeshComponent->SetOverlayMaterial(PreviousOverlayMaterial);
	}
	else
	{
		MeshComponent->SetOverlayMaterial(nullptr);
	}

	// 이전 오버레이 정보를 초기화합니다.
	PreviousOverlayMaterial = nullptr;
	PreviousMeshComponent = nullptr;
}

UMaterialInterface* UTargetingComponent::GetOverlayMaterialForTeam(AActor* Target) const
{
	if (!Target)
	{
		return nullptr;
	}

	// 팀 관계를 확인합니다.
	ETeamAttitude::Type Attitude = ULuxTeamStatics::GetTeamAttitude(GetOwner(), Target);

	// 팀 관계에 따른 적절한 오버레이 머테리얼을 반환합니다.
	switch (Attitude)
	{
	case ETeamAttitude::Hostile:
		return HostileOverlayMaterial;
	case ETeamAttitude::Friendly:
		return FriendlyOverlayMaterial;
	case ETeamAttitude::Neutral:
		return NeutralOverlayMaterial;
	default:
		return nullptr;
	}
}

void UTargetingComponent::SetDetectionType(ETargetingDetectionType NewDetectionType)
{
    // 런타임 오버라이드를 설정합니다.
    if (!bHasDetectionOverride || DetectionTypeOverride != NewDetectionType)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s][%s] DetectionType Override 변경: %s%d -> %d"),
            *GetName(), *ClientServerStatus,
            bHasDetectionOverride ? TEXT("") : TEXT("(no-override) "),
            (int32)(bHasDetectionOverride ? DetectionTypeOverride : DetectionType),
            (int32)NewDetectionType);
        DetectionTypeOverride = NewDetectionType;
        bHasDetectionOverride = true;
        
        // 클라이언트에서 서버로 동기화 요청
        if (GetOwner() && Cast<APawn>(GetOwner())->IsLocallyControlled())
        {
			Server_SetDetectionType(NewDetectionType);
        }
    }
}

void UTargetingComponent::ClearDetectionTypeOverride()
{
    if (bHasDetectionOverride)
    {
        UE_LOG(LogLux, Warning, TEXT("[%s][%s] DetectionType Override 해제: %d (override removed) -> default %d"),
				*GetName(), *ClientServerStatus, (int32)DetectionTypeOverride, (int32)DetectionType);
        bHasDetectionOverride = false;
    }
}

ETargetingDetectionType UTargetingComponent::GetActiveDetectionType() const
{
    return bHasDetectionOverride ? DetectionTypeOverride : DetectionType;
}

void UTargetingComponent::NotifyDetectionTypeChanged(ETargetingDetectionType NewDetectionType)
{
    // 클라이언트에서 서버로 RPC 호출
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        Server_SetDetectionType(NewDetectionType);
    }
}

void UTargetingComponent::Server_SetDetectionType_Implementation(ETargetingDetectionType NewDetectionType)
{
    // 서버에서 DetectionType 설정
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        DetectionTypeOverride = NewDetectionType;
        bHasDetectionOverride = true;
        
        UE_LOG(LogLux, Log, TEXT("[%s][Server] DetectionType 동기화 완료: %d"), 
            *GetName(), (int32)NewDetectionType);
        
        // 클라이언트에 확인 응답
        Client_ConfirmDetectionTypeChange(NewDetectionType, true);
    }
}

void UTargetingComponent::Client_ConfirmDetectionTypeChange_Implementation(ETargetingDetectionType NewDetectionType, bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogLux, Log, TEXT("[%s][Client] DetectionType 서버 동기화 확인: %d"), 
            *GetName(), (int32)NewDetectionType);
    }
    else
    {
        UE_LOG(LogLux, Warning, TEXT("[%s][Client] DetectionType 서버 동기화 실패: %d"), 
            *GetName(), (int32)NewDetectionType);
    }
}

void UTargetingComponent::SetDefaultTargetingFilters(const TArray<UTargetFilter*>& NewFilters)
{
    DefaultTargetingFilters.Empty();
    for (UTargetFilter* Filter : NewFilters)
    {
        if (::IsValid(Filter))
        {
            DefaultTargetingFilters.Add(Filter);
        }
    }
}

void UTargetingComponent::AddDefaultTargetingFilter(UTargetFilter* Filter)
{
    if (::IsValid(Filter) && !DefaultTargetingFilters.Contains(Filter))
    {
        DefaultTargetingFilters.Add(Filter);
    }
}

void UTargetingComponent::RemoveDefaultTargetingFilter(UTargetFilter* Filter)
{
    DefaultTargetingFilters.Remove(Filter);
}

void UTargetingComponent::ClearDefaultTargetingFilters()
{
    DefaultTargetingFilters.Empty();
}