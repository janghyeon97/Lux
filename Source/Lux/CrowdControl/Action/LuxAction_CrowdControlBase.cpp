

#include "LuxAction_CrowdControlBase.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "Character/LuxCharacter.h"
#include "LuxLogChannels.h"

ULuxAction_CrowdControlBase::ULuxAction_CrowdControlBase()
{
    InstancingPolicy = ELuxActionInstancingPolicy::InstancedPerActor;
    ActivationPolicy = ELuxActionActivationPolicy::OnGrantAndRemove;
}


void ULuxAction_CrowdControlBase::OnPhaseEnter(const FGameplayTag& PhaseTag, UActionSystemComponent& SourceASC)
{
    Super::OnPhaseEnter(PhaseTag, SourceASC);

    if (PhaseTag == LuxPhaseTags::Phase_Action_Begin)
    {
        if (AssociatedStateTag.IsValid())
        {
            SourceASC.OnGameplayTagStackChanged.AddDynamic(this, &ULuxAction_CrowdControlBase::OnStateTagRemoved);
        }
    }
}

void ULuxAction_CrowdControlBase::OnActionEnd(bool bIsCancelled)
{
    if (UActionSystemComponent* ASC = GetActionSystemComponent())
    {
        ASC->OnGameplayTagStackChanged.RemoveDynamic(this, &ULuxAction_CrowdControlBase::OnStateTagRemoved);
    }

    Super::OnActionEnd(bIsCancelled);
}

void ULuxAction_CrowdControlBase::OnStateTagRemoved(const FGameplayTag& Tag, int32 OldCount, int32 NewCount)
{
    // 제거된 태그가 내가 감시하던 태그와 일치하고 Tag 스택이 0 이하 라면
    if (Tag == AssociatedStateTag && NewCount <= 0)
    {
        EndAction();
    }
}