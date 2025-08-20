// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/LuxActionTagRelationshipMapping.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxActionTagRelationshipMapping)

void ULuxActionTagRelationshipMapping::GetBlockedActivationTags(const FGameplayTagContainer& ActionTags, FGameplayTagContainer& OutTagsToBlock) const
{
	for (const FActionTagRelationship& Tags : ActionTagRelationships)
	{
		if (ActionTags.HasTag(Tags.ActionTag))
		{
			OutTagsToBlock.AppendTags(Tags.ActivationBlockedTags);
		}
	}
}

void ULuxActionTagRelationshipMapping::GetRequiredActivationTags(const FGameplayTagContainer& ActionTags, FGameplayTagContainer& OutTagsToRequire) const
{
	for (const FActionTagRelationship& Tags : ActionTagRelationships)
	{
		if (ActionTags.HasTag(Tags.ActionTag))
		{
			OutTagsToRequire.AppendTags(Tags.ActivationRequiredTags);
		}
	}
}

void ULuxActionTagRelationshipMapping::GetCancelledByTags(const FGameplayTagContainer& ActionTags, FGameplayTagContainer& OutTagsToCancel) const
{
	for (const FActionTagRelationship& Tags : ActionTagRelationships)
	{
		if (ActionTags.HasTag(Tags.ActionTag))
		{
			OutTagsToCancel.AppendTags(Tags.ActionTagsToCancel);
		}
	}
}

bool ULuxActionTagRelationshipMapping::IsActionCancelledByTag(const FGameplayTagContainer& ActionTags, const FGameplayTag& ActionTag) const
{
	// Simple iteration for now
	for (int32 i = 0; i < ActionTagRelationships.Num(); i++)
	{
		const FActionTagRelationship& Tags = ActionTagRelationships[i];

		if (Tags.ActionTag == ActionTag && Tags.ActionTagsToCancel.HasAny(ActionTags))
		{
			return true;
		}
	}

	return false;
}