

#include "Actors/Action/Aurora/CryoseismExplosion.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Actions/LuxActionLevelData.h"
#include "ActionSystem/Actions/Aurora/AuroraAction_Cryoseism.h"
#include "ActionSystem/Effects/LuxEffect.h"

#include "System/LuxCombatManager.h"
#include "System/LuxAssetManager.h"
#include "Cues/LuxCueTags.h"
#include "Cues/LuxCueNotify.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "Teams/LuxTeamStatics.h"
#include "Engine/OverlapResult.h"


ACryoseismExplosion::ACryoseismExplosion()
{
	PrimaryActorTick.bCanEverTick = false;

	Level = 1;

	ExplodeTime = 0.0f;
	SlowDuration = 0.0f;
	ExplosionDamage = 0.0f;
	StunDuration = 0.0f;
	ChainStunDuration = 0.0f;
	InitialRadius = 0.0f;
	ChainRadius = 0.0f;

	InitialExplosionCue = LuxCueTags::Cue_Aurora_Ultimate_InitialBlast;
	ChainExplosionCue = LuxCueTags::Cue_Aurora_Ultimate_ExplodeChain;
	SlowEffectCue = LuxCueTags::Cue_Aurora_Ultimate_Slowed;
}

void ACryoseismExplosion::Initialize(ULuxAction* Action, bool bAutoStart)
{
	Super::Initialize(Action, bAutoStart);
	
	if (!Action)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: Initialize 실패 - Action이 유효하지 않습니다."));
		return;
	}

	if (!SourceASC.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: Initialize 실패 - ASC가 유효하지 않습니다."));
		return;
	}

	const FName RowName = FName(*FString::FromInt(ActionLevel));
	const FLuxActionLevelData* LevelData = Action->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("CryoseismExplosion Initialize"));

	if (!LevelData)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: LevelDataTable에서 레벨 %d 데이터를 찾을 수 없습니다."), ActionLevel);
		return;
	}

	// 구체적인 초기화 로직
	const FAuroraActionLevelData_Cryoseism* Data = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_Cryoseism>();
	if (!Data)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: Initialize 실패 - Data가 유효하지 않습니다."));
		return;
	}

	CollisionChannel = Data->CollisionChannel;
	ExplodeTime = Data->ExplodeTime;
	SlowDuration = Data->SlowDuration;
	SlowMagnitude = Data->SlowMagnitude;
	ExplosionDamage = Data->MagicalDamage;
	StunDuration = Data->StunDuration;
	ChainStunDuration = Data->ChainStunDuration;
	InitialRadius = Data->InitialRadius;
	ChainRadius = Data->ChainRadius;

	if (bAutoStart)
	{
		Start();
	}
}

void ACryoseismExplosion::BeginPlay()
{
	Super::BeginPlay();

}

void ACryoseismExplosion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ACryoseismExplosion::Start()
{
	Super::Start();

	if (HasAuthority())
	{
		bIsInitialExplosion = true;

		CueContext.Instigator = SourceASC->GetAvatarActor();
		CueContext.Location = SourceASC->GetAvatarActor()->GetActorLocation();
		CueContext.Rotation = SourceASC->GetAvatarActor()->GetActorRotation();
		CueContext.EffectCauser = SourceASC->GetOwnerActor();
		CueContext.SourceASC = SourceASC.Get();

		SourceASC->ExecuteGameplayCue(SourceASC->GetAvatarActor(), InitialExplosionCue, CueContext);

		// 초기 대상을 찾아 멤버 변수에 저장합니다.
		TargetsForNextExplosion = FindExplosionTargets(GetActorLocation(), InitialRadius);

		// 첫 폭발을 예약합니다.
		ApplySlowEffectToTargets(TargetsForNextExplosion);
		ScheduleExplosion();
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: 액션에 의한 엑터는 서버에서만 실행할 수 있습니다."));
		return;
	}
}

void ACryoseismExplosion::Stop()
{
	AlreadyAffectedTargets.Empty();
	TargetsForNextExplosion.Empty();

	Super::Stop();
}

TSet<TObjectPtr<AActor>> ACryoseismExplosion::FindExplosionTargets(const FVector& Origin, float Radius) const
{
	if (!SourceASC.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: 소유자 액션 컴포넌트가 유효하지 않습니다."));
		return {};
	}

	UWorld* World = GetWorld();
	AActor* AvatarActor = SourceASC->GetAvatarActor();
	if (!World || !AvatarActor)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: 월드 또는 아바타타 액터가 유효하지 않습니다."));
		return {};
	}

	//DrawDebugSphere(World, Origin, Radius, 16, FColor::Red, false, 10.0f);

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	World->OverlapMultiByChannel(OverlapResults, Origin, FQuat::Identity, CollisionChannel, FCollisionShape::MakeSphere(Radius), QueryParams);
	if (OverlapResults.Num() <= 0)
	{	
		UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: 폭발 범위 내 대상을 찾을 수 없습니다."));
		return {};
	}

	TSet<TObjectPtr<AActor>> FoundTargets;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor) continue;

		if (HitActor == AvatarActor) 
			continue;

		if(AlreadyAffectedTargets.Contains(HitActor)) 
			continue;

		/*
		if (ULuxTeamStatics::GetTeamAttitude(OwnerActor, HitActor) != ETeamAttitude::Hostile)
		{
			continue; // 적대적이지 않은 대상은 건너뜁니다.
		}
		*/

		UE_LOG(LogLuxActionSystem, Error, TEXT("Cryoseism 폭발 범위 내 대상 발견: %s"), *HitActor->GetName());
		FoundTargets.Add(HitActor);
	}

	return MoveTemp(FoundTargets);
}

void ACryoseismExplosion::ApplySlowEffectToTargets(const TSet<TObjectPtr<AActor>>& Targets) const
{
	if (!SourceASC.IsValid()) return;

	// TODO: 둔화 효과(Slow Effect) 적용 로직 구현
	for (AActor* Target : Targets)
	{
		IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(Target);
		if (!ASCInterface) continue;

		UActionSystemComponent* TargetASC = ASCInterface->GetActionSystemComponent();
		if (!TargetASC) continue;

		ULuxCombatManager* CombatManager = GetWorld()->GetSubsystem<ULuxCombatManager>();
		if (CombatManager)
		{
			CombatManager->ApplyCrowdControl(
				SourceASC.Get(),
				TargetASC,
				LuxGameplayTags::CrowdControl_Slow,
				SlowDuration,
				(1.0f - SlowMagnitude)
			);

			TargetASC->ExecuteGameplayCue(Target, SlowEffectCue, CueContext);
		}
	}
}

void ACryoseismExplosion::ScheduleExplosion()
{
	FTimerHandle TimerHandle;

	if (ExplodeTime <= 0.0f)
	{
		UE_LOG(LogLuxActionSystem, Warning, TEXT("CryoseismExplosion: ExplodeTime이 유효하지 않은 값(%.2f)입니다. 즉시 폭발을 실행합니다."), ExplodeTime);
		GetWorldTimerManager().SetTimerForNextTick(this, &ACryoseismExplosion::HandleExplosion);
		return;
	}

	GetWorldTimerManager().SetTimer(TimerHandle, this, &ACryoseismExplosion::HandleExplosion, ExplodeTime, false);
}

void ACryoseismExplosion::HandleExplosion()
{
	if (!SourceASC.IsValid()) return;

	for (AActor* Target : TargetsForNextExplosion)
	{
		if (AlreadyAffectedTargets.Contains(Target)) 
			continue;

		IActionSystemInterface* TargetASI = Cast<IActionSystemInterface>(Target);
		if (!TargetASI) continue;

		UActionSystemComponent* TargetASC = TargetASI->GetActionSystemComponent();
		if (!TargetASC) continue;

		ULuxCombatManager* CombatManager = GetWorld()->GetSubsystem<ULuxCombatManager>();
		if (!CombatManager)
		{
			UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: Failed to get CombatManager"));
			return;
		}

		TSubclassOf<ULuxEffect> DamageEffectClass = CombatManager->GetDefaultDamageEffect();
		if (!DamageEffectClass)
		{
			UE_LOG(LogLuxActionSystem, Error, TEXT("CryoseismExplosion: Failed to get DamageEffectClass"));
			return;
		}

		FLuxEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.SetTargetASC(TargetASC);
		Context.SetSourceAction(ActionSpecHandle);

		FLuxEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(DamageEffectClass, Level, Context);
		if (DamageSpec.IsValid())
		{
			DamageSpec.Get()->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Base, ExplosionDamage);
		}

		// 데미지 & 얼어붙음 효과 적용
		const float DurationToApply = bIsInitialExplosion ? StunDuration : ChainStunDuration;
		CombatManager->ApplyDamage(SourceASC.Get(), TargetASC, DamageSpec);
		CombatManager->ApplyCrowdControl(
			SourceASC.Get(),
			TargetASC,
			LuxGameplayTags::CrowdControl_Frozen,
			DurationToApply,
			0
		);

		// 시각 효과(Cue)를 재생합니다.
		FGameplayTag CueToPlay = bIsInitialExplosion ? InitialExplosionCue : ChainExplosionCue;
		if (CueToPlay.IsValid())
		{
			FLuxCueContext TargetCueContext;

			TargetCueContext.Instigator = CueContext.Instigator;
			TargetCueContext.EffectCauser = CueContext.EffectCauser;
			TargetCueContext.SourceASC = CueContext.SourceASC;

			TargetCueContext.Location = Target->GetActorLocation();
			TargetCueContext.Rotation = Target->GetActorRotation();

			SourceASC->ExecuteGameplayCue(Target, LuxCueTags::Cue_Aurora_Ultimate_Frozen, TargetCueContext);
			SourceASC->ExecuteGameplayCue(Target, CueToPlay, TargetCueContext);
		}

		// 처리된 대상으로 기록합니다.
		AlreadyAffectedTargets.Add(Target);
	}

	TSet<TObjectPtr<AActor>> NextChainTargets;
	for (AActor* Target : TargetsForNextExplosion)
	{
		TSet<TObjectPtr<AActor>> FoundTargets = FindExplosionTargets(Target->GetActorLocation(), ChainRadius);
		NextChainTargets.Append(FoundTargets);
	}

	TargetsForNextExplosion = NextChainTargets;
	bIsInitialExplosion = false;

	if (NextChainTargets.Num() > 0)
	{
		ChainExplosion();
	}
	else
	{
		Stop();
	}
}

void ACryoseismExplosion::ChainExplosion()
{
	ApplySlowEffectToTargets(TargetsForNextExplosion);
	ScheduleExplosion();
}