
#include "System/GameplayTagStack.h"
#include "ActionSystem/ActionSystemComponent.h" // 델리게이트 호출을 위해 포함
#include "LuxLogChannels.h"

FString FGameplayTagStack::GetDebugString() const
{
    return FString::Printf(TEXT("%s(%d)"), *Tag.ToString(), StackCount);
}

void FGameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
    if (StackCount <= 0)
    {
        return;
    }

    if (!Tag.IsValid())
    {
        // 유효하지 않은 태그에 대한 경고 로그
        return;
    }

    // 먼저 맵에서 찾아보고, 있으면 스택 수만 업데이트합니다.
    if (int32* ExistingCount = TagToCountMap.Find(Tag))
    {
        *ExistingCount += StackCount;

        // 배열에서도 찾아서 스택 수를 업데이트하고 Dirty로 마킹합니다.
        for (FGameplayTagStack& Stack : Stacks)
        {
            if (Stack.Tag == Tag)
            {
                Stack.StackCount = *ExistingCount;
                MarkItemDirty(Stack);
                return;
            }
        }
    }
    else // 맵에 없다면 새로운 스택입니다.
    {
        TagToCountMap.Add(Tag, StackCount);

        FGameplayTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);
        MarkArrayDirty();
    }
}

void FGameplayTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
    if (StackCount <= 0)
    {
        return;
    }

    if (!TagToCountMap.Contains(Tag))
    {
        return;
    }

    int32& ExistingCount = TagToCountMap.FindChecked(Tag);
    ExistingCount -= StackCount;

    if (ExistingCount <= 0)
    {
        // 스택이 0 이하가 되면 맵과 배열에서 모두 제거합니다.
        TagToCountMap.Remove(Tag);
        Stacks.RemoveAll([Tag](const FGameplayTagStack& Stack) { return Stack.Tag == Tag; });
        MarkArrayDirty();
    }
    else
    {
        // 스택이 남아있으면 배열을 찾아서 카운트만 업데이트합니다.
        for (FGameplayTagStack& Stack : Stacks)
        {
            if (Stack.Tag == Tag)
            {
                Stack.StackCount = ExistingCount;
                MarkItemDirty(Stack);
                break;
            }
        }
    }
}

int32 FGameplayTagStackContainer::GetStackCount(FGameplayTag Tag) const
{
    return TagToCountMap.FindRef(Tag);
}

bool FGameplayTagStackContainer::ContainsTag(FGameplayTag Tag) const
{
    if (TagToCountMap.Contains(Tag))
    {
        return true;
    }

    // 정확히 일치하는 태그가 없다면 부모-자식 관계를 확인합니다.
    for (const auto& TagCountPair : TagToCountMap)
    {
        const FGameplayTag& CurrentTag = TagCountPair.Key;
        if (CurrentTag.MatchesTag(Tag))
        {
            return true;
        }
    }

    return false;
}

FGameplayTagContainer FGameplayTagStackContainer::GetExplicitGameplayTags() const
{
    FGameplayTagContainer ExplicitTags;
    for (const auto& TagCountPair : TagToCountMap)
    {
        if (TagCountPair.Value > 0)
        {
            ExplicitTags.AddTag(TagCountPair.Key);
        }
    }
    return ExplicitTags;
}

void FGameplayTagStackContainer::Reset()
{
    TagToCountMap.Empty();
    Stacks.Empty();
    MarkArrayDirty();
}


// --- FastArraySerializer 콜백 함수들 ---

void FGameplayTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    const bool bIsClient = (OwnerComponent->GetOwnerRole() != ROLE_Authority);

    for (const int32 Index : RemovedIndices)
    {
        const FGameplayTagStack& Stack = Stacks[Index];
        TagToCountMap.Remove(Stack.Tag);

        // 태그가 제거되었음을 클라이언트의 다른 시스템에 알립니다.
        if (OwnerComponent.IsValid() && bIsClient)
        {
            OwnerComponent->OnGameplayTagStackChanged.Broadcast(Stack.Tag, Stack.StackCount, 0);
        }
    }
}

void FGameplayTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    const bool bIsClient = (OwnerComponent->GetOwnerRole() != ROLE_Authority);

    for (const int32 Index : AddedIndices)
    {
        const FGameplayTagStack& Stack = Stacks[Index];
        TagToCountMap.Add(Stack.Tag, Stack.StackCount);

        if (OwnerComponent.IsValid() && bIsClient)
        {
            OwnerComponent->OnGameplayTagStackChanged.Broadcast(Stack.Tag, 0, Stack.StackCount);
        }
    }
}

void FGameplayTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    const bool bIsClient = (OwnerComponent->GetOwnerRole() != ROLE_Authority);

    for (const int32 Index : ChangedIndices)
    {
        const FGameplayTagStack& Stack = Stacks[Index];

        if (OwnerComponent.IsValid() && bIsClient)
        {
            const int32 OldCount = TagToCountMap.FindRef(Stack.Tag);
            OwnerComponent->OnGameplayTagStackChanged.Broadcast(Stack.Tag, OldCount, Stack.StackCount);
        }

        TagToCountMap.FindOrAdd(Stack.Tag) = Stack.StackCount;
    }
}