// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "AuroraAction_Hoarfrost.generated.h"

class ULuxEffect;
class AHoarfrost;
class ULuxActionTask;

USTRUCT(BlueprintType)
struct FAuroraActionLevelData_Hoarfrost : public FActionLevelDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hoarfrost")
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECollisionChannel::ECC_GameTraceChannel7;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hoarfrost")
	float SnareDuration = 0.0f;
};

/**
 *
 */
UCLASS()
class LUX_API UAuroraAction_Hoarfrost : public ULuxAction
{
	GENERATED_BODY()

public:
	UAuroraAction_Hoarfrost();

protected:
	//~ ULuxAction Overrides
	virtual void OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC) override;
	virtual void OnActionEnd(bool bIsCancelled) override;
	//~ End ULuxAction Overrides

private:
	virtual void PhaseBegin(UActionSystemComponent& SourceASC);
	virtual void PhaseFreeze(UActionSystemComponent& SourceASC);
	virtual void PhaseRecovery(UActionSystemComponent& SourceASC);
	virtual void PhaseInterrupt(UActionSystemComponent& SourceASC);
	virtual void PhaseEnd(UActionSystemComponent& SourceASC);

	virtual void FreezeClient(UActionSystemComponent& SourceASC);
	virtual void FreezeServer(UActionSystemComponent& SourceASC);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AHoarfrost> HoarfrostClass;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AHoarfrost> HoarfrostActor;
};
