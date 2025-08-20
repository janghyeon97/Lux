// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetFilter.h"
#include "Teams/LuxTeamStatics.h"
#include "TeamTargetFilter.generated.h"

/**
 * 팀 관계를 기반으로 타겟을 필터링하는 필터입니다.
 * 특정 팀 관계(적대적/우호적/중립)의 타겟만 허용하거나 제외할 수 있습니다.
 */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Team Target Filter"))
class LUX_API UTeamTargetFilter : public UTargetFilter
{
	GENERATED_BODY()

public:
	UTeamTargetFilter();

	//~ UTargetFilter interface
	virtual bool IsTargetValid(AActor* Target, AActor* SourceActor) const override;
	virtual FString GetFilterDescription() const override;
	//~ End UTargetFilter interface

protected:
	/** 허용할 팀 관계들입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Filter")
	TArray<TEnumAsByte<ETeamAttitude::Type>> AllowedTeamAttitudes;

	/** 
	 * true면 AllowedTeamAttitudes에 포함된 팀만 허용 (화이트리스트)
	 * false면 AllowedTeamAttitudes에 포함된 팀을 제외 (블랙리스트)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Filter")
	bool bUseAsWhitelist = true;

	/** 타겟의 팀을 확인할 수 없는 경우 허용할지 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Filter")
	bool bAllowUnknownTeam = false;

	/** 디버그 정보를 로그로 출력할지 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Filter|Debug")
	bool bDebugLog = false;
};
