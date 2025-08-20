#pragma once

#include "CoreMinimal.h"
#include "Actors/LuxBaseActionActor.h"
#include "GameplayTagContainer.h"
#include "Cues/LuxCueNotify.h" 
#include "CryoseismExplosion.generated.h"

class ULuxAction;
class UActionSystemComponent;


/** 얼음 폭발을 담당하는 액터 클래스 */
UCLASS()
class LUX_API ACryoseismExplosion : public ALuxBaseActionActor
{
	GENERATED_BODY()

public:
	ACryoseismExplosion();

	/** 액션 객체에서 필요한 데이터만 추출해 Initialize를 호출합니다. */
	virtual void Initialize(ULuxAction* Action, bool bAutoStart) override;
	
	//~ ALuxBaseActionActor Overrides
	virtual void Start() override;
	virtual void Stop() override;
	//~ End of ALuxBaseActionActor Overrides

protected:
	// ~ AActor Overrides
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// ~ End of AActor Overrides

	/** 지정된 위치 주변의 적을 찾아 배열에 담아 반환합니다. */
	TSet<TObjectPtr<AActor>> FindExplosionTargets(const FVector& Origin, float Radius) const;

	/** 대상들에게 둔화 효과를 적용합니다. */
	void ApplySlowEffectToTargets(const TSet<TObjectPtr<AActor>>& Targets) const;

	/** 폭발을 예약합니다. */
	void ScheduleExplosion();

	/** 폭발 효과를 처리합니다. */
	void HandleExplosion();

	/** 연쇄 폭발을 처리합니다. */
	void ChainExplosion();

private:
	/** 연쇄 폭발 시 이미 피해를 입은 대상을 추적하기 위한 Set 입니다. */
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> AlreadyAffectedTargets;

	/** 다음 연쇄 폭발이 적용될 대상 목록입니다. */
	UPROPERTY()
	TSet<TObjectPtr<AActor>> TargetsForNextExplosion;

	/** 첫 폭발 시 재생할 Gameplay Cue 태그입니다. */
	UPROPERTY(EditAnywhere, Category = "Effects")
	FGameplayTag InitialExplosionCue;

	/** 연쇄 폭발 시 재생할 Gameplay Cue 태그입니다. */
	UPROPERTY(EditAnywhere, Category = "Effects")
	FGameplayTag ChainExplosionCue;

	/** 슬로우 효과 적용 시 재생할 Gameplay Cue 태그입니다. */
	UPROPERTY(EditAnywhere, Category = "Effects")
	FGameplayTag SlowEffectCue;

	UPROPERTY(EditAnywhere, Category = "Effects")
	FLuxCueContext CueContext;

	TEnumAsByte<ECollisionChannel> CollisionChannel;

	/** 첫 번째 폭발인지 여부를 추적하는 플래그입니다. */
	bool bIsInitialExplosion = true;

	int32 Level;

	float ExplosionDamage;

	float SlowDuration;
	float SlowMagnitude;
	float ExplodeTime;
	float StunDuration;
	float ChainStunDuration;
	float InitialRadius;
	float ChainRadius;
}; 