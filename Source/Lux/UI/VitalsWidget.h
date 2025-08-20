// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/LuxUserWidgetBase.h"
#include "VitalsWidget.generated.h"


class UAttributeBarBase;

/**
 * 
 */
UCLASS()
class LUX_API UVitalsWidget : public ULuxUserWidgetBase
{
	GENERATED_BODY()
	
public:
	// ~ ULuxUserWidgetBase overrides
	virtual void InitializeWidget(UActionSystemComponent* InASC) override;

protected:
	// ~ UUserWidget overrides
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/** Health Bar에 접근할 수 있도록 public으로 노출 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAttributeBarBase> HealthBar;

	/** Mana Bar에 접근할 수 있도록 public으로 노출 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAttributeBarBase> ManaBar;

	/** Experience Bar에 접근할 수 있도록 public으로 노출 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAttributeBarBase> ExperienceBar;
};
