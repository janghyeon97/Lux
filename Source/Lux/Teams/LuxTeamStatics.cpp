// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/LuxTeamStatics.h"
#include "LuxTeamAgentInterface.h"

ETeamAttitude::Type ULuxTeamStatics::GetTeamAttitude(const AActor* A, const AActor* B)
{
    if (A == nullptr || B == nullptr)
    {
        return ETeamAttitude::Neutral;
    }

    // 둘 중 하나가 IGenericTeamAgentInterface를 구현하지 않았다면 중립 관계입니다.
    const IGenericTeamAgentInterface* TeamAgentA = Cast<IGenericTeamAgentInterface>(A);
    const IGenericTeamAgentInterface* TeamAgentB = Cast<IGenericTeamAgentInterface>(B);
    if (TeamAgentA == NULL || TeamAgentB == NULL)
    {
        return ETeamAttitude::Neutral;
	}

    // 두 액터 모두에서 TeamID를 가져옵니다.
    FGenericTeamId TeamA = TeamAgentA->GetGenericTeamId();
    FGenericTeamId TeamB = TeamAgentB->GetGenericTeamId();

    // 어느 한쪽이라도 팀이 없다면 중립 관계입니다.
    if (TeamA == FGenericTeamId::NoTeam || TeamB == FGenericTeamId::NoTeam)
    {
        return ETeamAttitude::Neutral;
    }

    return FGenericTeamId::GetAttitude(TeamA, TeamB);
}