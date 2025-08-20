// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Effects/LuxEffect.h"
#include "LuxLogChannels.h"

AActor* FLuxEffectContext::GetInstigator() const
{
	return Instigator.IsValid() ? Instigator.Get() : nullptr;
}

AActor* FLuxEffectContext::GetEffectCauser() const
{
	return EffectCauser.IsValid() ? EffectCauser.Get() : nullptr;
}

UActionSystemComponent* FLuxEffectContext::GetTargetASC() const
{
	return TargetASC.IsValid() ? TargetASC.Get() : nullptr;
}

UActionSystemComponent* FLuxEffectContext::GetSourceASC() const
{
	return SourceASC.IsValid() ? SourceASC.Get() : nullptr;
}

FLuxActionSpecHandle FLuxEffectContext::GetSourceAction() const
{
	return SourceActionHandle;
}

void FLuxEffectContext::SetInstigator(AActor* InActor)
{
	Instigator = InActor;
}

void FLuxEffectContext::SetEffectCauser(AActor* InActor)
{
	EffectCauser = InActor;
}

void FLuxEffectContext::SetTargetASC(UActionSystemComponent* InASC)
{
	TargetASC = InASC;
}

void FLuxEffectContext::SetSourceASC(UActionSystemComponent* InASC)
{
	SourceASC = InASC;
}

void FLuxEffectContext::SetSourceAction(const FLuxActionSpecHandle& InHandle)
{
	SourceActionHandle = InHandle;
}

bool FLuxEffectContext::operator==(const FLuxEffectContext& Other) const
{
	if (Instigator != Other.Instigator)
	{
		return false;
	}

	if (EffectCauser != Other.EffectCauser)
	{
		return false;
	}

	if (TargetASC != Other.TargetASC)
	{
		return false;
	}

	if (SourceASC != Other.SourceASC)
	{
		return false;
	}

	return true;
}

bool FLuxEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 RepBits = 0;
	if (Ar.IsSaving())
	{
		if (Instigator.IsValid())
		{
			RepBits |= 1 << 0;
		}
		if (EffectCauser.IsValid())
		{
			RepBits |= 1 << 1;
		}
		if (TargetASC.IsValid())
		{
			RepBits |= 1 << 2;
		}
		if (SourceASC.IsValid())
		{
			RepBits |= 1 << 3;
		}
		if (SourceActionHandle.IsValid())
		{
			RepBits |= 1 << 4;
		}
	}

	Ar.SerializeBits(&RepBits, 5);

	if (RepBits & (1 << 0))
	{
		Ar << Instigator;
	}
	if (RepBits & (1 << 1))
	{
		Ar << EffectCauser;
	}
	if (RepBits & (1 << 2))
	{
		Ar << TargetASC;
	}
	if (RepBits & (1 << 3))
	{
		Ar << SourceASC;
	}
	if (RepBits & (1 << 4))
	{
		SourceActionHandle.NetSerialize(Ar, Map, bOutSuccess);;
	}

	bOutSuccess = true;
	return true;
}

// ------------------------------------------------------------------------------------------------

AActor* FLuxEffectContextHandle::GetInstigator() const
{
	return FLuxEffectContextHandle::IsValid() ? Context->GetInstigator() : nullptr;
}

AActor* FLuxEffectContextHandle::GetEffectCauser() const
{
	return FLuxEffectContextHandle::IsValid() ? Context->GetEffectCauser() : nullptr;
}

UActionSystemComponent* FLuxEffectContextHandle::GetTargetASC() const
{
	return FLuxEffectContextHandle::IsValid() ? Context->GetTargetASC() : nullptr;
}

UActionSystemComponent* FLuxEffectContextHandle::GetSourceASC() const
{
	return FLuxEffectContextHandle::IsValid() ? Context->GetSourceASC() : nullptr;
}

FLuxActionSpecHandle FLuxEffectContextHandle::GetSourceAction() const
{
	return FLuxEffectContextHandle::IsValid() ? Context->GetSourceAction() : FLuxActionSpecHandle();
}

void FLuxEffectContextHandle::SetInstigator(AActor* InActor)
{
	if (::IsValid(InActor))
	{
		Context->SetInstigator(InActor);
	}
}

void FLuxEffectContextHandle::SetEffectCauser(AActor* InActor)
{
	if (::IsValid(InActor))
	{
		Context->SetEffectCauser(InActor);
	}
}

void FLuxEffectContextHandle::SetTargetASC(UActionSystemComponent* InASC)
{
	if (::IsValid(InASC))
	{
		Context->SetTargetASC(InASC);
	}
}

void FLuxEffectContextHandle::SetSourceASC(UActionSystemComponent* InASC)
{
	if (::IsValid(InASC))
	{
		Context->SetSourceASC(InASC);
	}
}

void FLuxEffectContextHandle::SetSourceAction(const FLuxActionSpecHandle& InHandle)
{
	if (InHandle.IsValid())
	{
		Context->SetSourceAction(InHandle);
	}
}

bool FLuxEffectContextHandle::operator==(const FLuxEffectContextHandle& Other) const
{
	return Context == Other.Context;
}

bool FLuxEffectContextHandle::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 RepBits = 0;

	if (Ar.IsSaving())
	{
		bool bHasValidContext = Context.IsValid();
		Ar << bHasValidContext;

		if (bHasValidContext)
		{
			Ar << Context->Instigator;
			Ar << Context->EffectCauser;
			Ar << Context->TargetASC;
			Ar << Context->SourceASC;
			Context->SourceActionHandle.NetSerialize(Ar, Map, bOutSuccess);
		}
	}
	else
	{
		bool bHasValidContext = false;
		Ar << bHasValidContext;

		if (bHasValidContext)
		{
			NewContext();

			Ar << Context->Instigator;
			Ar << Context->EffectCauser;
			Ar << Context->TargetASC;
			Ar << Context->SourceASC;
			Context->SourceActionHandle.NetSerialize(Ar, Map, bOutSuccess);
		}
		else
		{
			Context.Reset();
		}
	}

	bOutSuccess = true;
	return true;
}


// ------------------------------------------------------------------------------------------------

FLuxEffectSpec::FLuxEffectSpec()
	: EffectTemplate(nullptr)
	, Level(1.0f)
{

}

FLuxEffectSpec::FLuxEffectSpec(const ULuxEffect* InEffectTemplate, float InLevel, const FLuxEffectContextHandle& InContextHandle)
	: EffectTemplate(InEffectTemplate)
	, ContextHandle(InContextHandle)
	, Level(InLevel)
{
	if (!IsValid())
	{
		return;
	}

	// Duration과 Period 초기화
	if (InEffectTemplate->DurationPolicy == ELuxEffectDurationPolicy::HasDuration)
	{
		const FLuxScalableFloat& DurationData = InEffectTemplate->Duration;
		if (DurationData.CalculationType == EValueCalculationType::Static)
		{
			CalculatedDuration = DurationData.StaticValue;
		}
		else
		{
			// SetByCaller는 나중에 SetByCallerMagnitude로 설정될 예정
			// 여기서는 0.f로 초기화 (의미있는 기본값이 있다면 그것을 사용)
			CalculatedDuration = 0.f;
		}

		const FLuxScalableFloat& PeriodData = InEffectTemplate->Period;
		if (PeriodData.CalculationType == EValueCalculationType::Static)
		{
			CalculatedPeriod = PeriodData.StaticValue;
		}
		else
		{
			CalculatedPeriod = 0.f;
		}
	}

	// 모든 Modifier를 CalculatedModifiers에 추가
	CalculatedModifiers = InEffectTemplate->Modifiers;
	DynamicEffectTags = InEffectTemplate->EffectTags;
	DynamicGrantedTags = InEffectTemplate->GrantedTags;
	ApplicationRequiredTags = InEffectTemplate->ApplicationRequiredTags;
	ApplicationBlockedTags = InEffectTemplate->ApplicationBlockedTags;
	RemoveEffectsWithTags = InEffectTemplate->RemoveEffectsWithTags;
}

FLuxEffectSpec::FLuxEffectSpec(const FLuxEffectSpec& Other)
	: EffectTemplate(Other.EffectTemplate)
	, ContextHandle(Other.ContextHandle)
	, CalculatedModifiers(Other.CalculatedModifiers)
	, SourceObject(Other.SourceObject)
	, DynamicEffectTags(Other.DynamicEffectTags)
	, DynamicGrantedTags(Other.DynamicGrantedTags)
	, ApplicationRequiredTags(Other.ApplicationRequiredTags)
	, ApplicationBlockedTags(Other.ApplicationBlockedTags)
	, RemoveEffectsWithTags(Other.RemoveEffectsWithTags)
	, Level(Other.Level)
	, CalculatedPeriod(Other.CalculatedPeriod)
	, CalculatedDuration(Other.CalculatedDuration)
{
}

bool FLuxEffectSpec::IsValid() const
{
	return EffectTemplate.IsValid() && ContextHandle.IsValid();
}

void FLuxEffectSpec::SetByCallerMagnitude(const FGameplayTag& Tag, float Value)
{
	if (!Tag.IsValid())
		return;

	// Duration 처리
	if (EffectTemplate->Duration.CalculationType == EValueCalculationType::SetByCaller && EffectTemplate->Duration.CallerTag == Tag)
	{
		CalculatedDuration = Value;
	}

	// Period 처리
	if (EffectTemplate->Period.CalculationType == EValueCalculationType::SetByCaller && EffectTemplate->Period.CallerTag == Tag)
	{
		CalculatedPeriod = Value;
	}

	// Modifiers 처리 - SetByCaller 값이 설정된 Modifier의 Magnitude 값을 런타임에 설정합니다.
	for (FAttributeModifier& Mod : CalculatedModifiers)
	{
		if (Mod.Magnitude.CalculationType == EValueCalculationType::SetByCaller && Mod.Magnitude.CallerTag == Tag)
		{
			Mod.Magnitude.StaticValue = Value;
		}
	}
}

float FLuxEffectSpec::GetByCallerMagnitude(FGameplayTag Tag, bool bWarnIfNotFound, float DefaultValue) const
{
	if (!Tag.IsValid())
	{
		return DefaultValue;
	}

	if (EffectTemplate->Duration.CalculationType == EValueCalculationType::SetByCaller && EffectTemplate->Duration.CallerTag == Tag)
	{
		return CalculatedDuration;
	}

	if (EffectTemplate->Period.CalculationType == EValueCalculationType::SetByCaller && EffectTemplate->Period.CallerTag == Tag)
	{
		return CalculatedPeriod;
	}

	// CalculatedModifiers 배열을 순회하며 일치하는 태그를 찾습니다.
	for (const FAttributeModifier& Mod : CalculatedModifiers)
	{
		if (Mod.Magnitude.CalculationType == EValueCalculationType::SetByCaller && Mod.Magnitude.CallerTag == Tag)
		{
			return Mod.Magnitude.StaticValue;
		}
	}

	// 순회가 끝날 때까지 태그를 찾지 못했다면, 경고 로그를 출력하고 기본값을 반환합니다.
	if (bWarnIfNotFound)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("GetSetByCallerMagnitude: Effect '%s'에서 Tag '%s'를 찾을 수 없습니다."),
			*GetNameSafe(EffectTemplate.Get()), *Tag.ToString());
	}

	return DefaultValue;
}

FActiveLuxEffectHandle FActiveLuxEffectHandle::GenerateNewHandle()
{
	FActiveLuxEffectHandle NewHandle;
	NewHandle.Handle = ++Counter;
	return NewHandle;
}

bool FActiveLuxEffectHandle::operator==(const FActiveLuxEffectHandle& Other) const
{
	return Handle == Other.Handle;
}

bool FActiveLuxEffectHandle::operator!=(const FActiveLuxEffectHandle& Other) const
{
	return Handle != Other.Handle;
}

FActiveLuxEffect::FActiveLuxEffect(const FLuxEffectSpec& InSpec)
	: Spec(InSpec)
{
	Handle = FActiveLuxEffectHandle::GenerateNewHandle();
}

bool FActiveLuxEffect::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Handle.Handle;

	Ar << Spec.EffectTemplate;
	Ar << Spec.Level;
	Ar << Spec.CalculatedPeriod;
	Ar << Spec.CalculatedDuration;

	// 모든 태그 컨테이너들을 직렬화
	if (!Spec.DynamicEffectTags.NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}
	if (!Spec.DynamicGrantedTags.NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}
	if (!Spec.ApplicationRequiredTags.NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}
	if (!Spec.ApplicationBlockedTags.NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}
	if (!Spec.RemoveEffectsWithTags.NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}

	if (Ar.IsSaving()) // 서버가 클라이언트로 데이터를 보낼 때
	{
		if (Spec.ContextHandle.IsValid())
		{
			Ar << Spec.ContextHandle.Get()->Instigator;
			Ar << Spec.ContextHandle.Get()->EffectCauser;
			Ar << Spec.ContextHandle.Get()->TargetASC;
			Ar << Spec.ContextHandle.Get()->SourceASC;
		}
	}
	else
	{
		Spec.ContextHandle.NewContext();

		Ar << Spec.ContextHandle.Get()->Instigator;
		Ar << Spec.ContextHandle.Get()->EffectCauser;
		Ar << Spec.ContextHandle.Get()->TargetASC;
		Ar << Spec.ContextHandle.Get()->SourceASC;
	}

	Ar << StartTime;
	Ar << EndTime;
	Ar << CurrentStacks;
	
	bOutSuccess = true;
	return true;
}

void FActiveLuxEffectsContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid()) 
		return;

	for (const int32 Index : AddedIndices)
	{
		FActiveLuxEffect& AddedEffect = Items[Index];
		if (!AddedEffect.Handle.IsValid()) continue;

		// 클라이언트에서 쿨다운 맵을 업데이트하도록 합니다.
        OwnerComponent->OnRep_EffectAdded(AddedEffect);
	}
}

void FActiveLuxEffectsContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid())
		return;

	for (const int32 Index : ChangedIndices)
	{
		FActiveLuxEffect& ChangedEffect = Items[Index];
		if (!ChangedEffect.Handle.IsValid()) continue;

		// 기존 쿨다운 정보를 제거하고 새로 추가하여 갱신합니다.
        OwnerComponent->OnRep_EffectRemoved(ChangedEffect);
        OwnerComponent->OnRep_EffectAdded(ChangedEffect);
	}
}

void FActiveLuxEffectsContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	if (!OwnerComponent.IsValid())
		return;

	for (const int32 Index : RemovedIndices)
	{
		FActiveLuxEffect& RemovedEffect = Items[Index];
		if (!RemovedEffect.Handle.IsValid()) continue;

		// 클라이언트에서 쿨다운 맵을 업데이트하도록 합니다.
		OwnerComponent->OnRep_EffectRemoved(RemovedEffect);
	}
}