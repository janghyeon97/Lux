// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "AuroraAction_GlacialCharge.generated.h"

class AGlacialPath;
class ULuxActionTask;
class ULuxCameraMode;

USTRUCT(BlueprintType)
struct FAuroraActionLevelData_GlacialCharge : public FActionLevelDataBase
{
	GENERATED_BODY()

public:
	/** 길이 완전히 생성되기까지 걸리는 총 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GlacialCharge")
	float CreationDuration = 2.0f;

	/** 길이 생성 완료 후 월드에 남아있을 시간 (대쉬 이후 재시전이 가능한 시간) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GlacialCharge")
	float PathLifetime = 3.0f;

	/** 한 Segment 당 길이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GlacialCharge")
	float SegmentLength = 50.0f;

	/** 사용할 스플라인 메시 (레벨마다 다른 메시 사용 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GlacialCharge")
	TSoftObjectPtr<UStaticMesh> SplineMesh;

	/** 사용할 스플라인 머티리얼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GlacialCharge")
	TSoftObjectPtr<UMaterialInterface> SplineMaterial;
};

/**
 * 
 */
UCLASS()
class LUX_API UAuroraAction_GlacialCharge : public ULuxAction
{
	GENERATED_BODY()
	
public:
	UAuroraAction_GlacialCharge();

protected:
	//~ ULuxAction Override
	virtual void OnActionEnd(bool bIsCancelled) override;
	virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC) override;
	//~ End of ULuxAction Override

protected:
	void PhaseAnalyze(UActionSystemComponent& SourceASC);
	void PhaseDash(UActionSystemComponent& SourceASC);
	void PhaseWaitRecast(UActionSystemComponent& SourceASC);
	void PhaseDestroyPath(UActionSystemComponent& SourceASC);
	void PhaseInterrupt(UActionSystemComponent& SourceASC);
	void PhaseEnd(UActionSystemComponent& SourceASC);

	void DashClient(UActionSystemComponent& SourceASC);
	void DashServer(UActionSystemComponent& SourceASC);

	UFUNCTION()
	void OnPathReady(const FGameplayTag& EventTag, const FContextPayload& Payload);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGlacialPath> GlacialPathClass;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AGlacialPath> GlacialPathActor;

	bool bIsCameraModePushed = false;
};
