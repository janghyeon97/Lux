// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/LuxActionSet.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Effects/LuxEffect.h"
#include "ActionSystem/ActionSystemComponent.h"

#include "LuxLogChannels.h"

void FLuxActionSet_GrantedHandles::AddActionSpecHandle(const FLuxActionSpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		LuxActionSpecHandles.Add(Handle);
	}
}

void FLuxActionSet_GrantedHandles::AddLuxEffectHandle(const FActiveLuxEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		LuxEffectHandles.Add(Handle);
	}
}

void FLuxActionSet_GrantedHandles::AddLuxAttributeSet(ULuxAttributeSet* Set)
{
	LuxAttributeSets.Add(Set);
}

void FLuxActionSet_GrantedHandles::TakeFromActionSystem(UActionSystemComponent* ASC)
{
	check(ASC);

	if (::IsValid(ASC->GetOwnerActor()) == false || !ASC->GetOwnerActor()->HasAuthority())
	{
		return;
	}

	for (const FLuxActionSpecHandle& Handle : LuxActionSpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveAction(Handle);
		}
	}

	for (const FActiveLuxEffectHandle& Handle : LuxEffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveEffect(Handle);
		}
	}

	for (ULuxAttributeSet* Set : LuxAttributeSets)
	{
		ASC->RemoveAttribute(Set);
	}

	LuxActionSpecHandles.Reset();
	LuxEffectHandles.Reset();
	LuxAttributeSets.Reset();
}

ULuxActionSet::ULuxActionSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULuxActionSet::GrantToActionSystem(UActionSystemComponent* ASC, FLuxActionSet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(ASC);

	if (::IsValid(ASC->GetOwnerActor()) == false || !ASC->GetOwnerActor()->HasAuthority())
	{
		return;
	}

	// AttributeSet 을 부여합니다.
	for (int32 SetIndex = 0; SetIndex < GrantedLuxAttributes.Num(); ++SetIndex)
	{
		const FLuxActionSet_LuxAttributeSet& SetToGrant = GrantedLuxAttributes[SetIndex];

		if (!IsValid(SetToGrant.AttributeSet))
		{
			UE_LOG(LogLux, Error, TEXT("GrantedAttributes[%d] on ability set [%s] is not valid"), SetIndex, *GetNameSafe(this));
			continue;
		}

		if (ASC->GetAttributeSet(SetToGrant.AttributeSet))
		{
			continue;
		}

		ULuxAttributeSet* NewSet = NewObject<ULuxAttributeSet>(ASC->GetOwnerActor(), SetToGrant.AttributeSet);
		ASC->AddAttributeSet(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddLuxAttributeSet(NewSet);
		}
	}

	// 액션을 부여합니다.
	for (int32 ActionIndex = 0; ActionIndex < GrantedLuxActions.Num(); ++ActionIndex)
	{
		const FLuxActionSet_LuxAction& ActionToGrant = GrantedLuxActions[ActionIndex];

		if (!::IsValid(ActionToGrant.Action))
		{
			UE_LOG(LogLux, Error, TEXT("GrantedGameplayAbilities[%d] on ability set [%s] is not valid."), ActionIndex, *GetNameSafe(this));
			continue;
		}

		ULuxAction* ActionCDO = ActionToGrant.Action->GetDefaultObject<ULuxAction>();
		FLuxActionSpec ActionSpec(ActionToGrant.Action, ActionToGrant.InputTag, ActionToGrant.ActionLevel);
		const FLuxActionSpecHandle LuxActionSpecHandle = ASC->GrantAction(ActionSpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddActionSpecHandle(LuxActionSpecHandle);
		}
	}

	// 이펙트를 부여합니다.
	for (int32 EffectIndex = 0; EffectIndex < GrantedLuxEffects.Num(); ++EffectIndex)
	{
		const FLuxActionSet_LuxEffect& EffectToGrant = GrantedLuxEffects[EffectIndex];

		if (!::IsValid(EffectToGrant.LuxEffect))
		{
			UE_LOG(LogLux, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid"), EffectIndex, *GetNameSafe(this));
			continue;
		}

		FLuxEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.SetTargetASC(ASC);

		FLuxEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectToGrant.LuxEffect, EffectToGrant.EffectLevel, EffectContext);
		FActiveLuxEffectHandle ActiveLuxEffectHandle; ASC->ApplyEffectSpecToSelf(SpecHandle, ActiveLuxEffectHandle);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddLuxEffectHandle(ActiveLuxEffectHandle);
		}
	}
}
