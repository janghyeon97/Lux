// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/LuxUserWidgetBase.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "AttributeBarBase.generated.h"

class UProgressBar;
class UBorder;
class UTextBlock;

struct FBoundDelegateInfo
{
	// 어떤 델리게이트 프로퍼티인지 (예: OnHealthChanged)
	FMulticastDelegateProperty* DelegateProperty = nullptr;

	// 바인딩된 실제 함수 정보
	FScriptDelegate Delegate;

	// 이 델리게이트를 소유한 객체 (예: UResourceSet 인스턴스)
	TWeakObjectPtr<UObject> OwningObject = nullptr;
};


/**
 *
 */
UCLASS()
class LUX_API UAttributeBarBase : public ULuxUserWidgetBase
{
	GENERATED_BODY()


public:
	UAttributeBarBase(const FObjectInitializer& ObjectInitializer);

	// ~ ULuxUserWidgetBase overrides
	virtual void InitializeWidget(UActionSystemComponent* InASC) override;

protected:
	// ~ UUserWidget overrides
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Attribute를 초기화하고 현재 값을 반환합니다. */
	virtual float InitializeAttribute(FLuxAttribute& AttributeToInitialize, FName BoundFunctionName);

	/** 현재 Attribute 값에 맞춰 ProgressBar와 Text를 업데이트하는 내부 함수입니다. */
	void UpdateBar();

	/** Attribute의 값이 변경될 때마다 호출되는 콜백 함수입니다. */
	UFUNCTION()
	void OnAttributeChanged(float OldValue, float NewValue);

	/** Max Attribute의 값이 변경될 때마다 호출되는 콜백 함수입니다. */
	UFUNCTION()
	void OnMaxAttributeChanged(float OldValue, float NewValue);

	/** Regeneration Attribute의 값이 변경될 때마다 호출되는 콜백 함수입니다. */
	UFUNCTION()
	void OnRegenerationAttributeChanged(float OldValue, float NewValue);

public:
	/** 속성의 현재 값을 시각적으로 표시하는 ProgressBar입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	/** 속성의 현재 값과 최대 값을 텍스트로 표시합니다. (예: "100 / 100") */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	/** 속성의 자동 회복 속도를 표시하는 텍스트입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RegenerationText;

protected:
	/** Bar 의 현재 값으로 표시할 Attribute입니다. (예: UResourceSet의 'Health' 속성) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AttributeBarBase")
	FLuxAttribute AttributeToTrack;

	/** Bar의 최대값으로 표시할 Attribute입니다. (예: UResourceSet의 'MaxHealth' 속성) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AttributeBarBase")
	FLuxAttribute MaxAttributeToTrack;

	/** 재생량을 표시할 때 사용할 Attribute입니다. (예: UResourceSet의 'Regeneration' 속성) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AttributeBarBase")
	FLuxAttribute RegenerationAttributeToTrack;

	/** Attribute 값 변경 델리게이트에 대한 핸들입니다. NativeDestruct에서 해제하기 위해 저장합니다. */
	TArray<FBoundDelegateInfo> BoundDelegates;

	/** 현재 Attribute 값입니다. */
	float CurrentValue = 0.0f;

	/** 최대 Attribute 값입니다. */
	float MaxValue = 0.0f;

	/** 재생 Attribute 값입니다. */
	float RegenerationValue = 0.0f;
};
