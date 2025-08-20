// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Engine/DataAsset.h"

#include "Attributes/LuxAttributeSet.h"
#include "Actions/LuxActionTypes.h"
#include "Effects/LuxEffectTypes.h"
#include "GameplayTagContainer.h"

#include "LuxActionSet.generated.h"

class UActionSystemComponent;
class ULuxAttributeSet;
class ULuxEffect;
class UNyxAction;

class UObject;

/** Lux 어빌리티를 부여하기 위한 구조체 */
USTRUCT(BlueprintType)
struct FLuxActionSet_LuxAction
{
	GENERATED_BODY()

public:
	// 부여할 Lux Action 클래스
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ULuxAction> Action = nullptr;

	// 부여할 어빌리티 레벨
	UPROPERTY(EditDefaultsOnly)
	int32 ActionLevel = 1;

	// 어빌리티 입력 처리에 사용할 태그
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Input"))
	FGameplayTag InputTag;
};


/** Lux 이펙트를 부여하기 위한 구조체 */
USTRUCT(BlueprintType)
struct FLuxActionSet_LuxEffect
{
	GENERATED_BODY()

public:
	// 부여할 Lux Effect 클래스
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ULuxEffect> LuxEffect = nullptr;

	// 부여할 이펙트 레벨
	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.0f;
};


/** 애트리뷰트 셋(Attribute Set)을 부여하기 위한 구조체 */
USTRUCT(BlueprintType)
struct FLuxActionSet_LuxAttributeSet
{
	GENERATED_BODY()

public:
	// 부여할 Attribute Set 클래스
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ULuxAttributeSet> AttributeSet;
};


/** 부여된 핸들을 저장하는 구조체 */
USTRUCT(BlueprintType)
struct FLuxActionSet_GrantedHandles
{
	GENERATED_BODY()

public:
	// 어빌리티 스펙 핸들 추가
	void AddActionSpecHandle(const FLuxActionSpecHandle& Handle);

	// Lux Effect 핸들 추가
	void AddLuxEffectHandle(const FActiveLuxEffectHandle& Handle);

	// 부여된 애트리뷰트 셋 포인터 추가
	void AddLuxAttributeSet(ULuxAttributeSet* Set);

	// Action System으로부터 부여된 항목 제거
	void TakeFromActionSystem(UActionSystemComponent* ASC);

protected:
	// 부여된 어빌리티 스펙 핸들 목록
	UPROPERTY()
	TArray<FLuxActionSpecHandle> LuxActionSpecHandles;

	// 부여된 Lux Effect 핸들 목록
	UPROPERTY()
	TArray<FActiveLuxEffectHandle> LuxEffectHandles;

	// 부여된 애트리뷰트 셋 포인터 목록
	UPROPERTY()
	TArray<TObjectPtr<ULuxAttributeSet>> LuxAttributeSets;
};


/**
 * ULuxActionSet
 *
 * Non-mutable 데이터 에셋으로, 어빌리티와 이펙트를 부여합니다.
 */
UCLASS(BlueprintType, Const)
class ULuxActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULuxActionSet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 대상 Action System Component에 이 어빌리티 세트를 부여합니다.
	// OutGrantedHandles에 핸들을 저장하여 나중에 제거할 수 있습니다.
	void GrantToActionSystem(UActionSystemComponent* ASC, FLuxActionSet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:
	// 부여할 Lux Abilities 목록
	UPROPERTY(EditDefaultsOnly, Category = "Lux Abilities", meta = (TitleProperty = Action))
	TArray<FLuxActionSet_LuxAction> GrantedLuxActions;

	// 부여할 Lux Effects 목록
	UPROPERTY(EditDefaultsOnly, Category = "Lux Effects", meta = (TitleProperty = LuxEffect))
	TArray<FLuxActionSet_LuxEffect> GrantedLuxEffects;

	// 부여할 Attribute Sets 목록
	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta = (TitleProperty = AttributeSet))
	TArray<FLuxActionSet_LuxAttributeSet> GrantedLuxAttributes;
};