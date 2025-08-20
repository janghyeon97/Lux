// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Tasks/LuxActionTask.h"
#include "ActionSystem/Actions/LuxDynamicValue.h"
#include "LuxActionTask_TraceAndBuildPath.generated.h"



/**
 * ULuxActionTask_TraceAndBuildPath 태스크를 위한 초기화 파라미터 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FTraceAndBuildPathParams : public FBaseLuxActionTaskParams
{
    GENERATED_BODY()

public:
    /** 지형을 탐색할 최대 거리입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pathfinding")
    FDynamicFloat TraceDistance;

    /** 탐색을 진행할 간격(단계)의 크기입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pathfinding")
    FDynamicFloat StepSize;

    /** 경로 생성을 중단할 최대 높이 차이입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pathfinding")
    FDynamicFloat HeightThreshold;

    /** 평지/오르막/내리막을 구분하는 기준 높이 차이입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pathfinding")
    FDynamicFloat MaxHeightDifference;
};



USTRUCT()
struct FTerrainSegment
{
    GENERATED_BODY()

    int32 StartIndex;
    int32 EndIndex;
    FString Type;

    FTerrainSegment(int32 InStart = 0, int32 InEnd = 0, const FString& InType = TEXT(""))
        : StartIndex(InStart), EndIndex(InEnd), Type(InType) {
    }
};

/**
 * 
 */
UCLASS()
class LUX_API ULuxActionTask_TraceAndBuildPath : public ULuxActionTask
{
	GENERATED_BODY()
	
public:
    /**
     * 지형 분석 및 경로 생성 태스크를 생성하고 즉시 활성화합니다.
     * @param InOwningAction 이 태스크를 소유하는 액션입니다.
     * @param InTraceDistance 지형을 탐색할 최대 거리입니다.
     * @param InStepSize 탐색을 진행할 간격(단계)의 크기입니다.
     * @param InHeightThreshold 경로 생성을 중단할 최대 높이 차이입니다.
     * @param InMaxHeightDifference 평지/오르막/내리막을 구분하는 기준 높이 차이입니다.
     */
    UFUNCTION(BlueprintCallable, Category = "LuxActionSystem|Task", meta = (DisplayName = "Trace And Build Path"))
    static ULuxActionTask_TraceAndBuildPath* TraceAndBuildPath(
        ULuxAction* InOwningAction,
        float InTraceDistance = 1200.f,
        float InStepSize = 50.f,
        float InHeightThreshold = 600.f,
        float InMaxHeightDifference = 100.f
    );

protected:
    // ~ULuxActionTask interface
    virtual void OnActivated() override;
    virtual void InitializeFromStruct(const FInstancedStruct& Struct) override;
    //~End of ULuxActionTask interface

private:
    // 경로 생성 로직 함수들
    TArray<FVector> TraceTerrainPath(const FVector& StartLocation, const FVector& ForwardDirection);
    TArray<FTerrainSegment> AnalyzeTerrainSegments(const TArray<FVector>& PathPoints);
    void ProcessTerrainSegments(TArray<FTerrainSegment>& TerrainSegments);
    void SmoothTerrainSegments(TArray<FVector>& PathPoints, const TArray<FTerrainSegment>& TerrainSegments);

    // 정적 생성 함수로부터 전달받은 파라미터를 저장할 멤버 변수들
    UPROPERTY()
    float TraceDistance;

    UPROPERTY()
    float StepSize;

    UPROPERTY()
    float HeightThreshold;

    UPROPERTY()
    float MaxHeightDifference;
};
