// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LuxActionTagRelationshipMapping.generated.h"

/** 특정 어빌리티 태그와 그 외 태그들의 관계를 정의하는 구조체 */
USTRUCT()
struct FActionTagRelationship
{
	GENERATED_BODY()

	/** 이 관계의 주인이 되는 태그입니다. 보통 이 태그 하나로, 어빌리티에 부여된 태그를 대표할 수 있습니다 */
	UPROPERTY(EditAnywhere, Category = Action, meta = (Categories = "Action.Type"))
	FGameplayTag ActionTag;

	/** 이 태그를 가진 어빌리티가 활성화될 때 취소할 다른 태그들 */
	UPROPERTY(EditAnywhere, Category = Action)
	FGameplayTagContainer ActionTagsToCancel;

	/** 이 태그를 가진 어빌리티를 활성화하는 데 일반적으로 추가되는 활성화 필요 태그들 */
	UPROPERTY(EditAnywhere, Category = Action)
	FGameplayTagContainer ActivationRequiredTags;

	/** 이 태그를 가진 어빌리티를 활성화하는 데 일반적으로 추가되는 활성화 차단 태그들 */
	UPROPERTY(EditAnywhere, Category = Action)
	FGameplayTagContainer ActivationBlockedTags;
};


/** 어빌리티 태그가 다른 어빌리티를 차단/취소하는 규칙을 정의하는 데이터 에셋 */
UCLASS()
class ULuxActionTagRelationshipMapping : public UDataAsset
{
	GENERATED_BODY()

private:
	/** 태그 간 관계 목록으로, 어떤 태그가 어떤 태그를 차단/취소하는지를 정의합니다 */
	UPROPERTY(EditAnywhere, Category = Action, meta = (TitleProperty = "ActionTag"))
	TArray<FActionTagRelationship> ActionTagRelationships;

public:
	/** 주어진 액션 태그에 의해 활성화가 '차단'되는 태그들을 가져옵니다. */
	void GetBlockedActivationTags(const FGameplayTagContainer& ActionTags, FGameplayTagContainer& OutTagsToBlock) const;

	/** 주어진 액션 태그를 활성화하는 데 '필요한' 태그들을 가져옵니다. */
	void GetRequiredActivationTags(const FGameplayTagContainer& ActionTags, FGameplayTagContainer& OutTagsToRequire) const;

	/** 주어진 액션 태그가 활성화될 때 '취소'시킬 태그들을 가져옵니다. */
	void GetCancelledByTags(const FGameplayTagContainer& ActionTags, FGameplayTagContainer& OutTagsToCancel) const;

	/** 특정 액션 태그에 의해 어빌리티 태그들이 취소되는지 여부를 반환합니다 */
	bool IsActionCancelledByTag(const FGameplayTagContainer& ActionTags, const FGameplayTag& ActionTag) const;
};