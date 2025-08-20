// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "MovementSet.generated.h"

/**
 * 
 */
UCLASS()
class LUX_API UMovementSet : public ULuxAttributeSet
{
	GENERATED_BODY()
	

public:
	UMovementSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Attribute Accessors ---
	ATTRIBUTE_ACCESSORS(UMovementSet, MoveSpeed)

	// --- Delegates for UI update ---
	UPROPERTY(BlueprintAssignable, Category = "Attributes|Delegates")
	FLuxAttributeChanged OnMoveSpeedChanged;

protected:
	/** CurrentValue 값이 변경되기 직전에 호출됩니다. */
	virtual void PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue) override;

	/** CurrentValue 값이 변경된 직후에 호출됩니다. */
	virtual void PostAttributeChange(const FLuxAttribute& Attribute, float OldValue, float NewValue) override;

	// --- Replicated Properties ---
	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FLuxAttributeData& OldValue);

protected:
	/** 캐릭터의 이동 속도입니다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Attributes|Movement")
	FLuxAttributeData MoveSpeed;
};
