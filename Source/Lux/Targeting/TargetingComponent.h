// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Engine/EngineTypes.h"
#include "Materials/MaterialInterface.h"

#include "Filters/TargetFilter.h"
#include "Teams/LuxTeamStatics.h"
#include "LuxTargetingTypes.h"
#include "TargetingComponent.generated.h"

// 전방선언
class ULuxTargetingData;


// 타겟 변경 이벤트를 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetChanged, AActor*, NewTarget);



UCLASS(meta=(BlueprintSpawnableComponent))
class LUX_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingComponent();

	/** 컴포넌트 초기화 시 호출됩니다. */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	/** 컴포넌트 초기화 시 타겟팅 데이터를 로드합니다. */
	UFUNCTION(BlueprintCallable, Category = "Targeting|Initialization")
	void InitializeTargetingData();

	/** 기본값을 설정합니다. */
	void SetDefaultValues();

    /** 오버랩 검출을 통해 타겟을 찾습니다. (여러 타겟 반환) */
	UFUNCTION(BlueprintCallable, Category = "Targeting|Find")
	void FindOverlapTarget(TArray<AActor*>& OutTargets, EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters);

	/** 라인 트레이스를 통해 타겟을 찾습니다. (여러 타겟 반환) */
	UFUNCTION(BlueprintCallable, Category = "Targeting|Find")
	void FindMultiLineTraceTarget(TArray<AActor*>& OutTargets, EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters);

	/** 스윕 트레이스를 통해 타겟을 찾습니다. (여러 타겟 반환) */
	UFUNCTION(BlueprintCallable, Category = "Targeting|Find")
	void FindSweepTraceTarget(TArray<AActor*>& OutTargets, EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters);

	/** 현재 설정된 검출 방식을 사용하여 타겟을 찾습니다. */
	UFUNCTION(BlueprintCallable, Category = "Targeting|Find")
	AActor* FindBestTarget(EAttitudeQuery AttitudeQuery, const TArray<UTargetFilter*>& AdditionalFilters);

    /** 현재 타겟 액터를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Targeting|Getters")
    AActor* GetCurrentTarget() const { return CurrentTarget; }

    /** 현재 설정된 검출 방식을 반환합니다. (오버라이드가 있으면 오버라이드 우선) */
    UFUNCTION(BlueprintPure, Category = "Targeting|Settings")
    ETargetingDetectionType GetCurrentDetectionType() const { return GetActiveDetectionType(); }

    /** 검출 방식 오버라이드를 설정합니다. (카메라 모드 등 런타임 오버라이드 용) */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetDetectionType(ETargetingDetectionType NewDetectionType);

    /** 검출 방식 오버라이드를 설정합니다. (명시적 API) */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetDetectionTypeOverride(ETargetingDetectionType NewDetectionType) { SetDetectionType(NewDetectionType); }

    /** 검출 방식 오버라이드를 해제합니다. (데이터 기본값으로 복귀) */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void ClearDetectionTypeOverride();

    // 클라이언트에서 서버로 DetectionType 변경 요청
    UFUNCTION(Server, Reliable)
    void Server_SetDetectionType(ETargetingDetectionType NewDetectionType);

    // 서버에서 클라이언트로 DetectionType 변경 확인
    UFUNCTION(Client, Reliable)
    void Client_ConfirmDetectionTypeChange(ETargetingDetectionType NewDetectionType, bool bSuccess);

    /** 타겟을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Target")
    void SetTarget(AActor* NewTarget);

    /** 타겟을 제거합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Target")
    void ClearTarget();

    /** 유효한 타겟이 있는지 확인합니다. */
    UFUNCTION(BlueprintPure, Category = "Targeting|Target")
    bool HasValidTarget() const;

    /** 타겟까지의 거리를 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Targeting|Target")
    float GetDistanceToTarget() const;

    /** 타겟까지의 방향을 반환합니다. */
    UFUNCTION(BlueprintPure, Category = "Targeting|Target")
    FVector GetDirectionToTarget() const;

    /** Detection 반지름을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetOverlapRadius(float NewRadius);

    /** 최대 타겟 거리를 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetTraceDistance(float NewDistance);

    /** Sweep 반지름을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetSweepRadius(float NewRadius);

    /** 우선순위 가중치들을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetPriorityWeights(float NewDistanceWeight, float NewAngleWeight, float NewSizeWeight);

    /** 타겟팅 기능을 활성화/비활성화합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetEnableTargeting(bool bEnable);

    /** 우선순위 선택 기능을 활성화/비활성화합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetEnablePrioritySelection(bool bEnable);

    /** 검출할 오브젝트 타입을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Settings")
    void SetTargetObjectTypes(const TArray<TEnumAsByte<EObjectTypeQuery>>& NewObjectTypes);

    /** 검출할 오브젝트 타입을 가져옵니다. */
    UFUNCTION(BlueprintPure, Category = "Targeting|Settings")
    const TArray<TEnumAsByte<EObjectTypeQuery>>& GetTargetObjectTypes() const { return TargetObjectTypes; }

    /** 기본 타겟팅 필터를 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Filters")
    void SetDefaultTargetingFilters(const TArray<UTargetFilter*>& NewFilters);

    /** 기본 타겟팅 필터를 가져옵니다. */
    UFUNCTION(BlueprintPure, Category = "Targeting|Filters")
    TArray<UTargetFilter*> GetDefaultTargetingFilters() const
    { 
        TArray<UTargetFilter*> Result;
        for (const TObjectPtr<UTargetFilter>& Filter : DefaultTargetingFilters)
        {
            if (::IsValid(Filter))
            {
                Result.Add(Filter.Get());
            }
        }
        return Result;
    }

    /** 기본 타겟팅 필터를 추가합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Filters")
    void AddDefaultTargetingFilter(UTargetFilter* Filter);

    /** 기본 타겟팅 필터를 제거합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Filters")
    void RemoveDefaultTargetingFilter(UTargetFilter* Filter);

    /** 기본 타겟팅 필터를 모두 제거합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Filters")
    void ClearDefaultTargetingFilters();

    /** 오버레이 머테리얼 기능을 활성화/비활성화합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Overlay")
    void SetEnableOverlayMaterial(bool bEnable);

    /** 오버레이 머테리얼이 활성화되어 있는지 확인합니다. */
    UFUNCTION(BlueprintPure, Category = "Targeting|Overlay")
    bool IsOverlayMaterialEnabled() const { return bEnableOverlayMaterial; }

    /** 적대적 팀 타겟용 오버레이 머테리얼을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Overlay")
    void SetHostileOverlayMaterial(UMaterialInterface* NewMaterial);

    /** 우호적 팀 타겟용 오버레이 머테리얼을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Overlay")
    void SetFriendlyOverlayMaterial(UMaterialInterface* NewMaterial);

    /** 중립 팀 타겟용 오버레이 머테리얼을 설정합니다. */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Overlay")
    void SetNeutralOverlayMaterial(UMaterialInterface* NewMaterial);

    /** 캐시된 PlayerController와 PlayerCameraManager를 무효화합니다. (성능 최적화용) */
    UFUNCTION(BlueprintCallable, Category = "Targeting|Performance")
    void InvalidateCachedReferences();

    /** 타겟 변경되었을 때 호출되는 이벤트입니다. */
    UPROPERTY(BlueprintAssignable, Category = "Targeting|Events")
    FOnTargetChanged OnTargetChanged;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** 매 틱 호출되어 조준점 아래의 타겟을 갱신합니다. */
    void UpdateTarget();

    /** PlayerController를 찾거나 캐시된 값을 반환합니다. */
    APlayerController* GetOrFindPlayerController();

    /** PlayerCameraManager를 찾거나 캐시된 값을 반환합니다. */
    APlayerCameraManager* GetOrFindPlayerCameraManager();

    /** 우선순위 기반으로 최적의 타겟을 선택합니다. */
    AActor* SelectBestTargetByPriority(const TArray<AActor*>& Candidates);

    /** 타겟이 유효한지 확인합니다. */
    bool IsValidTarget(AActor* Target) const;

    /** 현재 활성(기본값/오버라이드) DetectionType을 반환합니다. */
    ETargetingDetectionType GetActiveDetectionType() const;

    /** 타겟의 오버레이 머테리얼을 적용합니다. */
    void ApplyOverlayMaterial(AActor* Target);

    /** 타겟의 오버레이 머테리얼을 제거합니다. */
    void RemoveOverlayMaterial(AActor* Target);

    /** 팀 관계에 따른 적절한 오버레이 머테리얼을 반환합니다. */
    UMaterialInterface* GetOverlayMaterialForTeam(AActor* Target) const;

    /** DetectionType 변경 시 RPC 호출 */
    void NotifyDetectionTypeChanged(ETargetingDetectionType NewDetectionType);

private:
    /** 현재 타겟 액터를 저장합니다. */
    UPROPERTY(Transient)
    AActor* CurrentTarget = nullptr;

    /** 캐시된 PlayerController (성능 최적화용) */
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> CachedPlayerController;

    /** 캐시된 PlayerCameraManager (성능 최적화용) */
    UPROPERTY(Transient)
    TObjectPtr<APlayerCameraManager> CachedPlayerCameraManager;

    /** 타겟을 탐색할 최대 거리입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    float TraceDistance = 1000.f;

    /** Detection 반지름입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    float OverlapRadius = 2500.0f;

    /** Sweep Trace 방식의 스피어(Sphere) 반지름입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    float SweepRadius = 50.0f;

    /** 현재 사용 중인 검출 방식입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Settings")
    ETargetingDetectionType DetectionType = ETargetingDetectionType::LineTrace;

    /** 런타임 오버라이드 여부 */
    UPROPERTY(Replicated, Transient)
    bool bHasDetectionOverride = false;

    /** 런타임 오버라이드 값 */
    UPROPERTY(Replicated, Transient)
    ETargetingDetectionType DetectionTypeOverride = ETargetingDetectionType::LineTrace;

    /** 검출할 오브젝트 타입들입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Settings")
    TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

    /** 기본 타겟팅 필터들입니다. (런타임에 동적으로 변경 가능) */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTargetFilter>> DefaultTargetingFilters;

    /** 거리, 크기, 각도를 종합하여 타겟 우선순위를 계산하는 기능을 활성화합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Priority")
    bool bEnablePrioritySelection = true;

    /** 타겟팅 기능을 활성화합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Settings")
    bool bEnableTargeting = true;

    /** 거리에 대한 가중치입니다. (가까울수록 높음) */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Priority", meta = (EditCondition = "bEnablePrioritySelection"))
    float DistanceWeight = 0.5f;

    /** 화면 중앙과의 각도에 대한 가중치입니다. (중앙에 가까울수록 높음) */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Priority", meta = (EditCondition = "bEnablePrioritySelection"))
    float AngleWeight = 0.3f;

    /** 크기에 대한 가중치입니다. (클수록 높음) */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Priority", meta = (EditCondition = "bEnablePrioritySelection"))
    float SizeWeight = 0.2f;

    /** 오버레이 머테리얼 기능을 활성화합니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Overlay")
    bool bEnableOverlayMaterial = true;

    /** 적대적 팀 타겟용 오버레이 머테리얼입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Overlay", meta = (EditCondition = "bEnableOverlayMaterial"))
    UMaterialInterface* HostileOverlayMaterial = nullptr;

    /** 우호적 팀 타겟용 오버레이 머테리얼입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Overlay", meta = (EditCondition = "bEnableOverlayMaterial"))
    UMaterialInterface* FriendlyOverlayMaterial = nullptr;

    /** 중립 팀 타겟용 오버레이 머테리얼입니다. */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Overlay", meta = (EditCondition = "bEnableOverlayMaterial"))
    UMaterialInterface* NeutralOverlayMaterial = nullptr;

    /** 이전 타겟의 오버레이 머테리얼을 저장합니다. */
    UPROPERTY(Transient)
    UMaterialInterface* PreviousOverlayMaterial = nullptr;

    /** 이전 타겟의 메시 컴포넌트를 저장합니다. */
    UPROPERTY(Transient)
    UMeshComponent* PreviousMeshComponent = nullptr;

    /** 타겟 업데이트 빈도를 제어합니다. (초 단위) */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Performance")
    float TargetUpdateInterval = 0.1f;

    /** 타겟 변경 감지를 위한 임계값입니다. (cm 단위) */
    UPROPERTY(EditDefaultsOnly, Category = "Targeting|Stability")
    float TargetChangeThreshold = 50.0f;


    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    bool bDrawDebug = true;

    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    bool bDrawDebugPriority = true;

    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    bool bDrawDebugTrace = true;

private:
    /** 마지막 타겟 업데이트 시간을 저장합니다. */
    float LastTargetUpdateTime = 0.0f;

	FString ClientServerStatus = "Client";
};