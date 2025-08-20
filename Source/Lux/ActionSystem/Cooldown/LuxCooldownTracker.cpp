#include "ActionSystem/Cooldown/LuxCooldownTracker.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Effects/LuxEffectTypes.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "Net/UnrealNetwork.h"

ULuxCooldownTracker::ULuxCooldownTracker()
{
	// 생성자에서 Owner 설정 (가장 안전한 방법)
	Cooldowns.Owner = this;
}

void ULuxCooldownTracker::Initialize(UActionSystemComponent* InOwnerASC)
{
	OwnerASC = InOwnerASC;

	if (OwnerASC.IsValid() && OwnerASC->GetOwner() && OwnerASC->GetOwner()->HasAuthority())
	{
		OwnerASC->OnEffectAppliedNative.AddUObject(this, &ULuxCooldownTracker::OnCooldownEffectAdded);
		OwnerASC->OnEffectRemovedNative.AddUObject(this, &ULuxCooldownTracker::OnCooldownEffectRemoved);
	}

    Cooldowns.Owner = this;
}

void ULuxCooldownTracker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Cooldowns);
}

void ULuxCooldownTracker::PostNetReceive()
{
	Super::PostNetReceive();
	
	//// 클라이언트에서 복제 후 Owner 설정 (Initialize가 호출되지 않은 경우 대비)
	//if (!Cooldowns.Owner)
	//{
	//	Cooldowns.Owner = this;
	//	// 클라이언트에서 Owner 설정 완료
	//}
	//
	//// 클라이언트에서 OwnerASC 설정 
	//if (!OwnerASC.IsValid())
	//{
	//	// 소유 컴포넌트를 찾아서 설정
	//	if (UObject* Outer = GetOuter())
	//	{
	//		if (UActionSystemComponent* ASC = Cast<UActionSystemComponent>(Outer))
	//		{
	//			OwnerASC = ASC;
	//		}
	//	}
	//}
}

bool FCooldownEntry::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

	CooldownTag.NetSerialize(Ar, Map, bOutSuccess);
	Ar << StartTime;
	Ar << EndTime;
	Ar << Duration;

	return bOutSuccess;
}

void FCooldownContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
    if (!Owner) 
	{
		return;
	}

    for (int32 Index : AddedIndices)
    {
        if (!Items.IsValidIndex(Index)) 
			continue;

        Owner->HandleEntryAdded(Items[Index]);
    }
}

void FCooldownContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
    if (!Owner) 
	{
		return;
	}

    for (int32 Index : ChangedIndices)
    {
        if (!Items.IsValidIndex(Index)) 
			continue;
			
        Owner->HandleEntryChanged(Items[Index]);
    }
}

void FCooldownContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
    if (!Owner) 
	{
		return;
	}

    for (int32 Index : RemovedIndices)
    {
        if (!Items.IsValidIndex(Index)) 
			continue;

        Owner->HandleEntryRemoved(Items[Index].CooldownTag);
    }
}

// ===== 트래커 내부 콜백 → 델리게이트 브로드캐스트 =====
void ULuxCooldownTracker::HandleEntryAdded(const FCooldownEntry& Entry)
{
    if (OwnerASC.IsValid() && OwnerASC->GetOwnerRole() == ROLE_Authority)
    {
        return;
    }

    // 클라이언트에서 StartTime을 현재 클라이언트 시간으로 조정
    float ClientNow = 0.f;
    if (UWorld* World = GetWorld())
    {
        ClientNow = World->GetTimeSeconds();
    }
    
    // 복제된 엔트리의 StartTime을 클라이언트 시간으로 조정
    const int32 Index = FindCooldownIndex(Entry.CooldownTag);
    if (Index != INDEX_NONE)
    {
        FCooldownEntry& MutableEntry = Cooldowns.Items[Index];
        const float OriginalStartTime = MutableEntry.StartTime;
        MutableEntry.StartTime = ClientNow;
        MutableEntry.EndTime = ClientNow + Entry.Duration;
    }
    
	OnCooldownAdded.Broadcast(Entry.CooldownTag, Entry.Duration);
    OnCooldownChanged.Broadcast(Entry.CooldownTag, GetTimeRemaining(Entry.CooldownTag), Entry.Duration);
}

void ULuxCooldownTracker::HandleEntryChanged(const FCooldownEntry& Entry)
{
    if (OwnerASC.IsValid() && OwnerASC->GetOwnerRole() == ROLE_Authority)
    {
        return;
    }

	// 클라이언트 쿨다운 변경
    OnCooldownChanged.Broadcast(Entry.CooldownTag, GetTimeRemaining(Entry.CooldownTag), Entry.Duration);
}

void ULuxCooldownTracker::HandleEntryRemoved(const FGameplayTag& RemovedTag)
{
    if (OwnerASC.IsValid() && OwnerASC->GetOwnerRole() == ROLE_Authority)
    {
        return;
    }

	// 클라이언트 쿨다운 제거
    OnCooldownRemoved.Broadcast(RemovedTag);
}

void ULuxCooldownTracker::StartCooldown(const FGameplayTag& CooldownTag, float Duration)
{
	if (!OwnerASC.IsValid() || !OwnerASC->GetOwner() || !OwnerASC->GetOwner()->HasAuthority())
	{
		return;
	}

	const int32 ExistingIndex = FindCooldownIndex(CooldownTag);
	if (ExistingIndex != INDEX_NONE)
	{
		FCooldownEntry& Entry = Cooldowns.Items[ExistingIndex];
		Entry.StartTime = OwnerASC->GetWorld()->GetTimeSeconds();
		Entry.Duration = Duration;
		Entry.EndTime = Entry.StartTime + Duration;
		Cooldowns.MarkItemDirty(Entry);
		return;
	}

	FCooldownEntry& NewEntry = Cooldowns.Items.Emplace_GetRef();
	NewEntry.CooldownTag = CooldownTag;
	NewEntry.StartTime = OwnerASC->GetWorld()->GetTimeSeconds();
	NewEntry.Duration = Duration;
	NewEntry.EndTime = NewEntry.StartTime + Duration;
	Cooldowns.MarkItemDirty(NewEntry);
	Cooldowns.MarkArrayDirty();
}

void ULuxCooldownTracker::StopCooldown(const FGameplayTag& CooldownTag)
{
	if (!OwnerASC.IsValid() || !OwnerASC->GetOwner() || !OwnerASC->GetOwner()->HasAuthority())
	{
		return;
	}

	const int32 Index = FindCooldownIndex(CooldownTag);
	if (Index != INDEX_NONE)
	{
		Cooldowns.Items.RemoveAt(Index);
		Cooldowns.MarkArrayDirty();
	}
}

void ULuxCooldownTracker::OnCooldownEffectAdded(const FActiveLuxEffect& Effect)
{
	// Effect가 쿨다운 효과인지 확인 후 등록
	if (Effect.Spec.DynamicEffectTags.HasTag(LuxGameplayTags::Effect_Type_Cooldown) == false)
	{
		return;
	}

	// 부여 태그 중 첫 번째 Action.Cooldown.* 태그를 키로 사용
	for (const FGameplayTag& GrantedTag : Effect.Spec.DynamicGrantedTags)
	{
		if (GrantedTag.MatchesTag(LuxGameplayTags::Action_Cooldown) == false)
		{
			continue;
		}

		const float Duration = Effect.Spec.CalculatedDuration;

		// 쿨다운 효과 추가
		StartCooldown(GrantedTag, Duration);
		break;
	}
}

void ULuxCooldownTracker::OnCooldownEffectRemoved(const FActiveLuxEffect& Effect)
{
	if (Effect.Spec.DynamicEffectTags.HasTag(LuxGameplayTags::Effect_Type_Cooldown) == false)
	{
		return;
	}

	for (const FGameplayTag& GrantedTag : Effect.Spec.DynamicGrantedTags)
	{
		if (GrantedTag.MatchesTag(LuxGameplayTags::Action_Cooldown) == false)
		{
			continue;
		}

		StopCooldown(GrantedTag);
		break;
	}
}

float ULuxCooldownTracker::GetTimeRemaining(const FGameplayTag& CooldownTag) const
{
	const int32 Index = FindCooldownIndex(CooldownTag);
	if (Index == INDEX_NONE)
	{
		return 0.f;
	}

	const FCooldownEntry& Entry = Cooldowns.Items[Index];
	
	// 서버와 클라이언트 시간 기준점이 다를 수 있으므로 상대적 계산 사용
	if (OwnerASC.IsValid() && OwnerASC->GetOwnerRole() == ROLE_Authority)
	{
		// 서버에서는 절대 시간 사용
		const float Now = OwnerASC->GetWorld() ? OwnerASC->GetWorld()->GetTimeSeconds() : 0.f;
		const float TimeRemaining = FMath::Max(0.f, Entry.EndTime - Now);
		
		// 서버 시간 계산
		return TimeRemaining;
	}
	else
	{
		// 클라이언트에서는 상대적 계산 (Duration 기준)
		float Now = 0.f;
		if (OwnerASC.IsValid() && OwnerASC->GetWorld())
		{
			Now = OwnerASC->GetWorld()->GetTimeSeconds();
		}
		else if (UWorld* World = GetWorld())
		{
			Now = World->GetTimeSeconds();
		}
		else
		{
			UE_LOG(LogLuxCooldown, Error, TEXT("[%s] 클라이언트에서 현재 시간을 가져올 수 없습니다!"), ANSI_TO_TCHAR(__FUNCTION__));
			return Entry.Duration; // 전체 지속시간 반환
		}
		
		// 클라이언트에서는 조정된 시간으로 계산
		const float TimeRemaining = FMath::Max(0.f, Entry.EndTime - Now);
		
		// 클라이언트 시간 계산
		return TimeRemaining;
	}
}

float ULuxCooldownTracker::GetDuration(const FGameplayTag& CooldownTag) const
{
	const int32 Index = FindCooldownIndex(CooldownTag);
	if (Index == INDEX_NONE)
	{
		return 0.f;
	}
	return Cooldowns.Items[Index].Duration;
}

void ULuxCooldownTracker::ReduceCooldown(const FGameplayTag& CooldownTag, float Seconds)
{
	if (!OwnerASC.IsValid() || !OwnerASC->GetOwner() || !OwnerASC->GetOwner()->HasAuthority())
	{
		return;
	}

    const int32 Index = FindCooldownIndex(CooldownTag);
	if (Index == INDEX_NONE)
	{
		return;
	}

	FCooldownEntry& Entry = Cooldowns.Items[Index];
	const float Now = OwnerASC->GetWorld()->GetTimeSeconds();
	const float Remaining = FMath::Max(0.f, Entry.EndTime - Now);
	const float NewRemaining = FMath::Max(0.f, Remaining - FMath::Max(0.f, Seconds));
	Entry.EndTime = Now + NewRemaining;
	//Entry.Duration = FMath::Max(Entry.Duration - Seconds, 0.f);
	Cooldowns.MarkItemDirty(Entry);

    // ASC의 실제 만료 타이머도 재설정
    if (UActionSystemComponent* ASC = OwnerASC.Get())
    {
        ASC->AdjustCooldownTimerByTag(CooldownTag, NewRemaining, Now);
    }
}

void ULuxCooldownTracker::ReduceCooldownByPercent(const FGameplayTag& CooldownTag, float Percent)
{
	ReduceCooldown(CooldownTag, GetTimeRemaining(CooldownTag) * FMath::Clamp(Percent, 0.f, 1.f));
}

int32 ULuxCooldownTracker::FindCooldownIndex(const FGameplayTag& Tag) const
{
	for (int32 i = 0; i < Cooldowns.Items.Num(); ++i)
	{
		if (Cooldowns.Items[i].CooldownTag == Tag)
		{
			return i;
		}
	}
	return INDEX_NONE;
}


