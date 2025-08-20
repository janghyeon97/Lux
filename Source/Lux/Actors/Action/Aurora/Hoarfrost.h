// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/LuxBaseActionActor.h"

#include "Hoarfrost.generated.h"


class UBoxComponent;


/** 
 * 얼음 고리의 충돌 처리를 담당하는 액터 클래스 
 */
UCLASS()
class LUX_API AHoarfrost : public ALuxBaseActionActor
{
	GENERATED_BODY()
	
public:	
	AHoarfrost();

	/** 액션 객체로부터 필요한 정보를 추출해 초기화합니다. */
    virtual void Initialize(ULuxAction* Action, bool bAutoStart) override;

protected:
	// ~ AActor interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	// ~ End of AActor interface

	void InitializeBoxes();

	UFUNCTION()
	void OnSegmentOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSegmentTick();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hoarfrost|Components")
	TArray<TObjectPtr<UBoxComponent>> CollisionBoxes;

	// 중복 처리를 막기 위한 Set
	TSet<TWeakObjectPtr<AActor>> ProcessedActors;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize OriginLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator OriginRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PhysicalDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MagicalDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PhysicalScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MagicalScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SnareDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumSegments = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Rate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Angle = 0.0f;

private:
	FVector ForwardVector = FVector::ZeroVector;
	FVector UpVector = FVector::ZeroVector;

	float TimeSinceLastSpawn = 0.0f;
	float CurrentAngleDeg = 0.0f;

	int32 CurrentSegmentIndex = 0;
};
