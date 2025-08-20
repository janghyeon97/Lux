// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "AuroraAction_FrozenSimulacrum.generated.h"

class UAnimMontage;
class AFrozenSimulacrum;


USTRUCT(BlueprintType)
struct FAuroraActionLevelData_FrozenSimulacrum : public FActionLevelDataBase
{
	GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FrozenSimulacrum")
    float LeapDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FrozenSimulacrum")
	float LeapDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FrozenSimulacrum")
	float LeapHeight = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FrozenSimulacrum")
    float LeapHeightThreshold = 600.0f;

    /** 분신이 월드에 유지되는 시간 (초) 입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FrozenSimulacrum")
    float SimulacrumDuration = 7.0f;

    /** 분신의 기본 최대 체력입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FrozenSimulacrum")
    float SimulacrumBaseMaxHealth = 100.0f;

    /** 시전자의 최대 체력에 비례하여 추가될 분신의 체력 계수입니다. (예: 0.5 = 시전자 최대 체력의 50%) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FrozenSimulacrum")
    float SimulacrumHealthScale_CasterMaxHealth = 0.5f;
};



/**
 * 
 */
UCLASS()
class LUX_API UAuroraAction_FrozenSimulacrum : public ULuxAction
{
	GENERATED_BODY()
	
public:
    UAuroraAction_FrozenSimulacrum();

protected:
    // ~ ULuxAction Overrides
    virtual void OnActionEnd(bool bIsCancelled) override;
    virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC) override;
    // ~ End ULuxAction Overrides

private:
    void PhaseLeap(UActionSystemComponent& SourceASC);
    void PhaseRecovery(UActionSystemComponent& SourceASC);
    void PhaseInterrupt(UActionSystemComponent& SourceASC);
    void PhaseEnd(UActionSystemComponent& SourceASC);

    /** 캐릭터의 이동 방향을 8방향 인덱스로 계산합니다. */
    int32 CalculateDirectionIndex(ACharacter* TargetCharacter) const;

    FVector CalculateLeapDestination(ACharacter* Character, const FVector& Direction, float Range, float HeightThreshold);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> MontageToPlay;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor")
    TSubclassOf<AFrozenSimulacrum> SimulacrumClass;

    UPROPERTY(Transient)
    AFrozenSimulacrum* FrozenSimulacrum;
};
