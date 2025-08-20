// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/ActorInitData.h"
#include "Actors/LuxBaseActionActor.h"
#include "GlacialPath.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;


UCLASS()
class LUX_API AGlacialPath : public ALuxBaseActionActor
{
	GENERATED_BODY()
	
public:
	AGlacialPath();

	/** 액션 객체에서 필요한 데이터만 추출해 Initialize를 호출합니다. */
	virtual void Initialize(ULuxAction* Action, bool bAutoStart = true) override;

	UFUNCTION(BlueprintCallable, Category = "GlacialPath")
	USplineComponent* GetSplineComponent() const { return SplineComponent; }

protected:
	// ~ AActor interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	// ~ End of AActor interface

	/** 모든 클라이언트에서 실행되어 스플라인을 생성하고 메시 생성을 시작하는 RPC 함수입니다. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BuildSpline(const FAuroraActionLevelData_GlacialCharge& Data, const TArray<FVector>& PathPoints, bool bAutoStart);

	/** 타이머에 의해 주기적으로 호출되어 스플라인 메시를 하나씩 생성합니다. */
	UFUNCTION()
	void UpdateSplineMesh(float DeltaTime);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> SplineComponent;

	/** 얼음 길 조각으로 사용할 스태틱 메시입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UStaticMesh> SplineStaticMesh;

	/** 얼음 길 조각에 적용할 머티리얼입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UMaterialInterface> SplineMaterial;

private:
	/** 생성된 스플라인 메시 컴포넌트들을 저장하는 배열입니다. */
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> SplineMeshComponents;

	/** 메시를 생성하기 위한 타이머 핸들입니다. */
	FTimerHandle BuildTimerHandle;

	/** 현재까지 생성된 스플라인 메시의 인덱스입니다. */
	int32 CurrentSegmentIndex = 0;

	/** 스플라인의 총 길이입니다. */
	float SplineLength = 0.0f;

	/** 스플라인의 총 세그먼트 수입니다. */
	int32 SegmentCount = 0;

	/** 세그먼트 하나의 길이입니다. */
	int32 SegmentLength = 0;

	/** 스플라인의 초기화 데이터입니다. */
	float DistanceAlongSpline = 0.0f;

	/** 생성 후 경과한 시간. */
	float ElapsedTime = 0.0f;

	/** 길이 완전히 생성되기까지 걸리는 총 시간 */
	float CreationDuration = 2.0f;
};
