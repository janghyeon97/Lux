// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "AuroraAction_Cryoseism.generated.h"


class ACryoseismExplosion;


USTRUCT(BlueprintType)
struct FAuroraActionLevelData_Cryoseism : public FActionLevelDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECollisionChannel::ECC_WorldStatic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float SlowDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"), Category = "Cryoseism")
	float SlowMagnitude = 0.4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float ExplodeTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float StunDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float ChainStunDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float InitialRadius = 850.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float ChainRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float LeapVelocity = -800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float LandingTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cryoseism")
	float LandingVelocity = -800.0f;
};


/**
 * 
 */
UCLASS()
class LUX_API UAuroraAction_Cryoseism : public ULuxAction
{
	GENERATED_BODY()
	
public:
	UAuroraAction_Cryoseism();

protected:
	virtual void OnActionEnd(bool bIsCancelled) override;
	virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC) override;

private:
	void PhaseLeap(UActionSystemComponent& SourceASC);
	void PhaseImpact(UActionSystemComponent& SourceASC);
	void PhaseLanding(UActionSystemComponent& SourceASC);
	void PhaseRecovery(UActionSystemComponent& SourceASC);
	void PhaseInterrupt(UActionSystemComponent& SourceASC);
	void PhaseEnd(UActionSystemComponent& SourceASC);

	/** 다음 노티파이까지의 시간을 계산하는 함수입니다. */
	float CalculateTimeToNextNotify(FName NotifyName);

	/** 땅까지의 거리를 계산하는 함수입니다. */
	float CalculateDistanceToGround(ACharacter* Character);

	/** 중력 가속도 고려 없이 일정한 속도로 착지하기 위한 속도를 계산하는 함수입니다. */
	float CalculateConstantVelocity(float Distance, float Time);

	/** 필요한 착지 속도를 계산하는 함수입니다. */
	float CalculateRequiredVelocity(float StartHeight, float LandingTime);

	/** 중력을 비활성화하는 함수입니다. */
	void DisableGravity();

	/** 중력을 재활성화하는 함수입니다. */
	void EnableGravity();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACryoseismExplosion> ExplosionClass;

private:
	/** 원래 애니메이션 속도를 저장합니다. */
	float OriginalPlayRate = 1.0f;

	/** 애니메이션 속도가 조절되었는지 확인합니다. */
	bool bAnimationSpeedAdjusted = false;

	/** 원래 중력 스케일을 저장합니다. */
	float OriginalGravityScale = 1.0f;

	/** 중력이 비활성화되었는지 확인합니다. */
	bool bGravityDisabled = false;
};
