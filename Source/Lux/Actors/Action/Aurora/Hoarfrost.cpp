// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Action/Aurora/Hoarfrost.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/Actions/LuxAction.h" 
#include "ActionSystem/Actions/Aurora/AuroraAction_Hoarfrost.h"
#include "ActionSystem/Effects/LuxEffect.h"

#include "System/LuxCombatManager.h"
#include "System/LuxAssetManager.h"

#include "Cues/LuxCueTags.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "Teams/LuxTeamStatics.h"

#include "Components/BoxComponent.h"


AHoarfrost::AHoarfrost()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AHoarfrost::Initialize(ULuxAction* Action, bool bAutoStart)
{
	Super::Initialize(Action, bAutoStart);

	if (!Action)
	{
		UE_LOG(LogLux, Error, TEXT("Hoarfrost: Initialize 실패 - Action이이 유효하지 않습니다."));
		Stop();
		return;
	}

	if (!SourceASC.IsValid())
	{
		UE_LOG(LogLux, Error, TEXT("Hoarfrost: Initialize 실패 - ASC 가 유효하지 않습니다."));
		Stop();
		return;
	}

	if(!Action->LevelDataTable)
	{
		UE_LOG(LogLux, Error, TEXT("Hoarfrost: Initialize 실패 - LevelDataTable이 유효하지 않습니다."));
		Stop();
		return;
	}

	const FName RowName = FName(*FString::FromInt(ActionLevel));
	const FLuxActionLevelData* LevelData = Action->LevelDataTable->FindRow<FLuxActionLevelData>(RowName, TEXT("Hoarfrost InitializeFromAction"));

	if (!LevelData)
	{
		UE_LOG(LogLux, Error, TEXT("Hoarfrost: LevelDataTable에서 레벨 %d 데이터를 찾을 수 없습니다."), ActionLevel);
		Stop();
		return;
	}

	const FAuroraActionLevelData_Hoarfrost* Data = LevelData->ActionSpecificData.GetPtr<FAuroraActionLevelData_Hoarfrost>();
	if (!Data)
	{
		UE_LOG(LogLux, Error, TEXT("Hoarfrost: LevelDataTable에서 레벨 %d 데이터를 찾을 수 없습니다."), ActionLevel);
		Stop();
		return;
	}

	this->PhysicalDamage = Data->PhysicalDamage;
	this->MagicalDamage = Data->MagicalDamage;
	this->PhysicalScale = Data->PhysicalScale;
	this->MagicalScale = Data->MagicalScale;
	this->SnareDuration = Data->SnareDuration;
	this->NumSegments = Data->Count;
	this->Rate = Data->Rate;
	this->Radius = Data->Radius;
	this->LifeTime = Data->Duration;
	this->Angle = 360.f / static_cast<float>(Data->Count);

	AActor* AvatarActor = SourceASC->GetAvatarActor();
	if(AvatarActor)
	{
		OriginLocation = AvatarActor->GetActorLocation();
		OriginRotation = AvatarActor->GetActorRotation();
		ForwardVector = OriginRotation.Vector();
		UpVector = OriginRotation.Quaternion().GetUpVector();
	}
	else
	{
		OriginLocation = GetActorLocation();
		OriginRotation = GetActorRotation();
		ForwardVector = OriginRotation.Vector();
		UpVector = OriginRotation.Quaternion().GetUpVector();
	}

	InitializeBoxes();

	if (bAutoStart)
	{
		Start();
	}
}

void AHoarfrost::InitializeBoxes()
{
	for (int i = 0; i <= NumSegments; i++)
	{
		FString BoxName = FString::Printf(TEXT("FreezeSegmentBox_%d"), i);
		UBoxComponent* BoxComponent = NewObject<UBoxComponent>(this, FName(*BoxName));
		if (!BoxComponent) continue;

		BoxComponent->SetupAttachment(this->GetRootComponent());
		//BoxComponent->IgnoreActorWhenMoving(this, true);
		BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		BoxComponent->SetBoxExtent(FVector(50.f));

		BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSegmentOverlap);
		BoxComponent->RegisterComponent();

		CollisionBoxes.Add(BoxComponent);
	}

}

void AHoarfrost::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 모든 박스 컴포넌트의 오버랩 이벤트를 제거합니다.
	for (UBoxComponent* Box : CollisionBoxes)
	{
		if (Box)
		{
			Box->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnSegmentOverlap);
			Box->DestroyComponent();
		}
	}
	CollisionBoxes.Empty();
}

void AHoarfrost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    TimeSinceLastSpawn += DeltaTime;
    if (TimeSinceLastSpawn >= Rate)
    {
        TimeSinceLastSpawn = 0.0f;
		OnSegmentTick();
    }
}

void AHoarfrost::OnSegmentTick()
{
	if (!SourceASC.IsValid())
	{
		SetActorTickEnabled(false);
		return;
	}

	AActor* AvatarActor = SourceASC->GetAvatarActor();
	if (!AvatarActor)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (CurrentSegmentIndex >= NumSegments)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (CollisionBoxes.IsValidIndex(CurrentSegmentIndex) == false)
	{
		CurrentSegmentIndex++;
		return;
	}

	UBoxComponent* CurrentBox = CollisionBoxes[CurrentSegmentIndex];
	if (!CurrentBox)
	{
		CurrentSegmentIndex++;
		return;
	}

	CurrentAngleDeg = CurrentSegmentIndex * Angle;
	const FVector HorizontalSpawnLocation = OriginLocation + ForwardVector.RotateAngleAxis(CurrentAngleDeg, UpVector) * Radius;
	FVector FinalSpawnLocation = HorizontalSpawnLocation;
	FRotator FinalSpawnRotation = ForwardVector.Rotation();
	FinalSpawnRotation.Yaw += CurrentAngleDeg + 90;

	UWorld* World = GetWorld();
	if (World)
	{
		const FVector TraceStart = HorizontalSpawnLocation + FVector(0.f, 0.f, 1000.f);
		const FVector TraceEnd = HorizontalSpawnLocation - FVector(0.f, 0.f, 1000.f);

		FHitResult HitResult;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(AvatarActor);

		if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, /*GroundTrace*/ECC_GameTraceChannel11, CollisionParams))
		{
			// 땅을 찾았다면 충돌 위치를 최종 스폰 위치로 사용합니다.
			FinalSpawnLocation = HitResult.Location;
			FinalSpawnRotation = FRotationMatrix::MakeFromZ(HitResult.ImpactNormal).Rotator();
			FinalSpawnRotation.Yaw = OriginRotation.Yaw + CurrentAngleDeg + 90;
		}
		else
		{
			// 땅을 못찾으면 박스를 활성화하지 않고 넘어갑니다.
			CurrentSegmentIndex++;
			return;
		}
	}

	CurrentBox->SetWorldLocationAndRotation(FinalSpawnLocation, FinalSpawnRotation);

	CurrentBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 쿼리(오버랩, 트레이스)만 가능하도록 설정
	CurrentBox->SetCollisionObjectType(ECC_GameTraceChannel7);     // 이 박스의 오브젝트 타입을 'DetectPlayer'로 설정

	// 모든 채널에 대한 반응을 먼저 '무시'로 초기화합니다.
	CurrentBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 오버랩을 원하는 특정 채널들에 대해서만 'Overlap'으로 설정합니다.
	CurrentBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap); // Player
	CurrentBox->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);	// Minion
	CurrentBox->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);	// Monster

	CurrentBox->SetGenerateOverlapEvents(true);

	//DrawDebugBox(World, FinalSpawnLocation, FVector(50), FColor::Red, false, 5.0f, 0, 2.0f);
	CurrentSegmentIndex++;
}
    
void AHoarfrost::OnSegmentOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!SourceASC.IsValid())
	{
		return;
	}

	AActor* AvatarActor = SourceASC->GetAvatarActor();
	if (!AvatarActor || !OtherActor /*|| OtherActor == AvatarActor*/ || ProcessedActors.Contains(OtherActor))
	{
		return;
	}

	// 적대적인 관계일 때만 로직을 실행합니다.
	/*if (ULuxTeamStatics::GetTeamAttitude(AvatarActor, OtherActor) != ETeamAttitude::Hostile)
	{
		return;
	}*/

	UE_LOG(LogLuxActionSystem, Warning, TEXT("[%s] 얼음 장판에 다른 캐릭터 '%s' 가 오버랩되었습니다. "), ANSI_TO_TCHAR(__FUNCTION__), *OtherActor->GetName());

	// 액션 시스템 인터페이스가 없는 액터는 무시합니다.
	IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(OtherActor);
	if (!ASCInterface)
	{
		return;
	}

	UActionSystemComponent* TargetASC = ASCInterface->GetActionSystemComponent();
	if (!TargetASC || TargetASC == SourceASC)
	{
		return;
	}

	ULuxCombatManager* CombatManager = GetWorld()->GetSubsystem<ULuxCombatManager>();
	if (!CombatManager) return;

	TSubclassOf<ULuxEffect> DamageLE = CombatManager->GetDefaultDamageEffect();
	if (!DamageLE)
	{
		return;
	}

	FLuxEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.SetInstigator(AvatarActor);
	EffectContext.SetEffectCauser(AvatarActor);
	EffectContext.SetSourceASC(SourceASC.Get());
	EffectContext.SetTargetASC(TargetASC);
	EffectContext.SetSourceAction(ActionSpecHandle);

	FLuxEffectSpecHandle DamageHandle = SourceASC->MakeOutgoingSpec(DamageLE, Level, EffectContext);
	FLuxEffectSpec* DamageSpec = DamageHandle.Get();
	if (DamageSpec)
	{
		DamageSpec->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Base, PhysicalDamage);
		DamageSpec->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Base,  MagicalDamage);
		DamageSpec->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Physical_Scale, PhysicalScale);
		DamageSpec->SetByCallerMagnitude(LuxGameplayTags::Effect_SetByCaller_Magical_Scale, MagicalScale);
	}

	FLuxCueContext CueContext;
	CueContext.Instigator = AvatarActor;
	CueContext.Location = AvatarActor->GetActorLocation();
	CueContext.Rotation = AvatarActor->GetActorRotation();

	CueContext.EffectCauser = AvatarActor;
	CueContext.SourceASC = SourceASC;

	CueContext.Count = 1;
	CueContext.Level = 1;
	CueContext.Lifetime = SnareDuration;
	CueContext.CueTags.AddTag(LuxGameplayTags::CrowdControl_Snare);

	CombatManager->ApplyDamage(SourceASC.Get(), TargetASC, DamageHandle);
	CombatManager->ApplyCrowdControl(SourceASC.Get(), TargetASC, LuxGameplayTags::CrowdControl_Snare, SnareDuration, 0);
	SourceASC->ExecuteGameplayCue(OtherActor, LuxCueTags::Cue_Aurora_Hoarfrost_Rooted, CueContext);
	ProcessedActors.Add(OtherActor);
}