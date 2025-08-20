// Copyright Epic Games, Inc. All Rights Reserved.

#include "TeamTargetFilter.h"
#include "Teams/LuxTeamStatics.h"
#include "LuxLogChannels.h"
#include "GameFramework/Actor.h"

UTeamTargetFilter::UTeamTargetFilter()
{
	// 기본값: 적대적 팀만 허용
	AllowedTeamAttitudes = { ETeamAttitude::Hostile };
	bUseAsWhitelist = true;
	bAllowUnknownTeam = false;
	bDebugLog = false;
}

bool UTeamTargetFilter::IsTargetValid(AActor* Target, AActor* SourceActor) const
{
	if (::IsValid(Target) == false)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[TeamTargetFilter] Target is null"));
		}
		return false;
	}

	// 소스 액터가 없으면 팀 관계를 확인할 수 없음
	if (::IsValid(SourceActor) == false)
	{
		if (bDebugLog)
		{
			UE_LOG(LogLux, Warning, TEXT("[TeamTargetFilter] Owner is null"));
		}
		return bAllowUnknownTeam;
	}

	// 팀 관계를 확인
	ETeamAttitude::Type TeamAttitude = ULuxTeamStatics::GetTeamAttitude(SourceActor, Target);
	
	if (bDebugLog)
	{
		FString AttitudeString;
		switch (TeamAttitude)
		{
		case ETeamAttitude::Friendly: AttitudeString = TEXT("Friendly"); break;
		case ETeamAttitude::Neutral: AttitudeString = TEXT("Neutral"); break;
		case ETeamAttitude::Hostile: AttitudeString = TEXT("Hostile"); break;
		default: AttitudeString = TEXT("Unknown"); break;
		}
		
		UE_LOG(LogLux, Log, TEXT("[TeamTargetFilter] %s -> %s: %s"), 
			*SourceActor->GetName(), *Target->GetName(), *AttitudeString);
	}

	// 허용된 팀 관계인지 확인
	bool bIsAllowedAttitude = AllowedTeamAttitudes.Contains(TeamAttitude);

	if (bUseAsWhitelist)
	{
		// 화이트리스트: 허용된 팀만 통과
		return bIsAllowedAttitude;
	}
	else
	{
		// 블랙리스트: 허용된 팀은 차단
		return !bIsAllowedAttitude;
	}
}

FString UTeamTargetFilter::GetFilterDescription() const
{
	FString Description = bUseAsWhitelist ? TEXT("Allow: ") : TEXT("Block: ");
	
	for (int32 i = 0; i < AllowedTeamAttitudes.Num(); ++i)
	{
		switch (AllowedTeamAttitudes[i])
		{
		case ETeamAttitude::Friendly:
			Description += TEXT("Friendly");
			break;
		case ETeamAttitude::Neutral:
			Description += TEXT("Neutral");
			break;
		case ETeamAttitude::Hostile:
			Description += TEXT("Hostile");
			break;
		default:
			Description += TEXT("Unknown");
			break;
		}
		
		if (i < AllowedTeamAttitudes.Num() - 1)
		{
			Description += TEXT(", ");
		}
	}
	
	return Description;
}
