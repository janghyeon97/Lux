// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "LuxInputConfig.generated.h"


class UObject;
class UInputAction;
struct FGameplayTag;


USTRUCT(BlueprintType)
struct FLuxInputAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	const UInputAction* InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};


/**
 * 
 */
UCLASS(BlueprintType, Const)
class LUX_API ULuxInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	ULuxInputConfig(const FObjectInitializer& ObjectInitializer);

	// 주어진 태그에 해당하는 네이티브 입력 액션을 찾아 반환합니다. (못 찾으면 로그 출력)
	UFUNCTION(BlueprintCallable, Category = "Lux|Pawn")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	// 주어진 태그에 해당하는 어빌리티 입력 액션을 찾아 반환합니다. (못 찾으면 로그 출력)
	UFUNCTION(BlueprintCallable, Category = "Lux|Pawn")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

public:
	// 디자이너가 정의하는 네이티브 입력 액션 목록입니다. 이 입력 액션들은 게임플레이 태그에 바인딩되며 수동으로 처리되어야 합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FLuxInputAction> NativeInputActions;

	// 디자이너가 정의하는 어빌리티 입력 액션 목록입니다. 이 입력 액션들은 게임플레이 태그에 바인딩되며, 매칭되는 입력 태그를 가진 어빌리티가 자동적으로 바인딩됩니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FLuxInputAction> AbilityInputActions;
};
