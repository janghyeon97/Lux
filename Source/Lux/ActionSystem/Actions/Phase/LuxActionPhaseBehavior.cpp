

#include "ActionSystem/Actions/Phase/LuxActionPhaseBehavior.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Effects/LuxEffect.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxActionLevelData.h"
#include "Actors/LuxActionSpawnedActorInterface.h"

#include "Character/LuxHeroCharacter.h"
#include "Camera/LuxCameraComponent.h" 
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

void FPhaseBehavior_AddTags::Execute(ULuxAction* Action) const
{
	if(!Action)
	{
		return;
	}

	Action->AddTags(TagsToAdd, 1);
}

void FPhaseBehavior_RemoveTags::Execute(ULuxAction* Action) const
{
	if(!Action)
	{
		return;
	}

	Action->RemoveTags(TagsToRemove, 1);
}

void FPhaseBehavior_RunTask::Execute(ULuxAction* Action) const
{
	if (!TaskToRun.TaskClass)
	{
		return;
	}

	ULuxActionTask* NewTask = NewObject<ULuxActionTask>(Action, TaskToRun.TaskClass);
	if(!NewTask)
	{
		return;
	}

	NewTask->OwningAction = Action;
	NewTask->InitializeFromStruct(TaskToRun.InitializationParameters);
	NewTask->Activate();

	Action->ActiveTasks.Add(NewTask);
}

void FPhaseBehavior_ApplyEffectToSelf::Execute(ULuxAction* Action) const
{
	if(!EffectToApply)
		return;

	UActionSystemComponent* ASC = Action->GetActionSystemComponent();
	if (!ASC) return;

	FActiveLuxAction* ActiveAction = ASC->FindActiveAction(Action->GetActiveHandle());
	if (!ActiveAction) return;

	FLuxEffectContextHandle Context = ASC->MakeEffectContext();
	Context.SetSourceAction(ActiveAction->Spec.Handle);

	FLuxEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectToApply, ActiveAction->Spec.Level, Context);
	if (SpecHandle.IsValid())
	{
		FActiveLuxEffectHandle Tmp; 
		ASC->ApplyEffectSpecToSelf(SpecHandle, Tmp);
	}
}

void FPhaseBehavior_PlaySound::Execute(ULuxAction* Action) const
{

}

void FPhaseBehavior_ExecuteCue::Execute(ULuxAction* Action) const
{
	if (!CueTag.IsValid())
		return;

	UActionSystemComponent* ASC = Action->GetActionSystemComponent();
	if (!ASC) return;

	AActor* Avatar = ASC->GetAvatarActor();
	if (!Avatar) return;

	AActor* Owner = ASC->GetOwnerActor();
	if (!Owner) return;

	FLuxCueContext Context;

	FVector LocOffset = LocationOffset.GetValue(Action);
	FRotator RotOffset = RotationOffset.GetValue(Action);

	// 오프셋의 기준 공간을 설정합니다.
	const FTransform AvatarTransform = Avatar->GetActorTransform();
	FVector FinalLocation = AvatarTransform.GetLocation();
	FRotator FinalRotation = AvatarTransform.GetRotation().Rotator();

	if (LocationOffset.Space == EPhaseParameterSpace::Local)
	{
		// 로컬 공간 - 아바타의 현재 방향을 기준으로 오프셋을 변환하여 더합니다.
		FinalLocation += AvatarTransform.TransformVector(LocOffset);
	}
	else
	{
		// 월드 공간
		FinalLocation += LocOffset;
	}

	if (RotationOffset.Space == EPhaseParameterSpace::Local)
	{
		// 로컬 공간 - 아바타의 현재 회전 값에 오프셋을 더합니다.
		FinalRotation = (AvatarTransform.GetRotation() * FQuat(RotOffset)).Rotator();
	}
	else
	{
		// 월드 공간
		FinalRotation = (FQuat(FinalRotation) * FQuat(RotOffset)).Rotator();
	}


	// 시전자, 위치 등 표준 런타임 정보를 채웁니다.
	Context.Instigator = Avatar;
	Context.EffectCauser = Action->GetOwnerActor();
	Context.SourceASC = ASC;
	Context.Location = FinalLocation;
	Context.Rotation = FinalRotation;
	Context.Normal = FinalRotation.Vector();

	Context.Magnitude = Magnitude.GetValue(Action);
	Context.Duration = Duration.GetValue(Action);
	Context.Rate = Rate.GetValue(Action);
	Context.Lifetime = Lifetime.GetValue(Action);

	const FActiveLuxAction* ActiveAction = ASC->FindActiveAction(Action->GetActiveHandle());
	if (ActiveAction)
	{
		Context.Level = ActiveAction->Spec.Level;
	}

	ASC->ExecuteGameplayCue(Avatar, CueTag, Context);
}

void FPhaseBehavior_EndAction::Execute(ULuxAction* Action) const
{
	if (!Action) return;

	Action->EndAction();
}

void FPhaseBehavior_StoreTaskResult::Execute(ULuxAction* Action) const
{
	if (Action && TriggeringEventTag.IsValid() && !SourcePayloadKey.IsNone())
	{
		const FName DestKey = DestinationPayloadKey.IsNone() ? SourcePayloadKey : DestinationPayloadKey;
		Action->AddTaskResultStoreRequest(TriggeringEventTag, SourcePayloadKey, DestKey);
	}
}

void FPhaseBehavior_SpawnActionActor::Execute(ULuxAction* Action) const
{
	if (!Action || !ActorClass) return;

	UWorld* World = Action->GetWorld();
	AActor* Avatar = Action->GetAvatarActor();
	if (!World || !Avatar) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Action->GetOwnerActor();
	SpawnParams.Instigator = Avatar->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = World->SpawnActor<AActor>(ActorClass, Avatar->GetActorLocation(), Avatar->GetActorRotation(), SpawnParams);
	if (SpawnedActor)
	{
		// ILuxActionSpawnedActorInterface를 구현한 액터라면 액션 시스템 연동
		if (ILuxActionSpawnedActorInterface* ActionInterface = Cast<ILuxActionSpawnedActorInterface>(SpawnedActor))
		{
			ActionInterface->Initialize(Action, true);
		}
		else
		{
			// 일반 AActor 처리 (예: 단순 이펙트, 데코레이션 등)
			UE_LOG(LogLuxActionSystem, Warning, TEXT("Spawned actor does not implement ILuxActionSpawnedActorInterface. Skipping action system logic."));
		}

		if (bDestroyWithAction)
		{
			Action->AddSpawnedActor(SpawnedActor);
		}
	}
}


void FPhaseBehavior_PushCameraMode::Execute(ULuxAction * Action) const
{
	if (!Action || !CameraModeToPush)
	{
		return;
	}

	ALuxHeroCharacter* Hero = Cast<ALuxHeroCharacter>(Action->GetAvatarActor());
	if (!Hero)
	{
		return;
	}

	ULuxCameraComponent* CameraComponent = Hero->GetCameraComponent();
	if (!CameraComponent)
	{
		return;
	}

	CameraComponent->PushCameraMode(CameraModeToPush);
	Action->PushedCameraModes.Add(CameraModeToPush);
	UE_LOG(LogLuxActionSystem, Error, TEXT("[%s] Action [%s] PushedCameraModes [%s] 추가, 현재 푸시된 모드 수: %d"), *Action->ClientServerString, *Action->GetName(), *CameraModeToPush->GetName(), Action->PushedCameraModes.Num());
}


void FPhaseBehavior_PopCameraMode::Execute(ULuxAction* Action) const
{
	if (!Action)
	{
		return;
	}

	ALuxHeroCharacter* Hero = Cast<ALuxHeroCharacter>(Action->GetAvatarActor());
	if (!Hero)
	{
		return;
	}

	ULuxCameraComponent* CameraComponent = Hero->GetCameraComponent();
	if (!CameraComponent)
	{
		return;
	}

	CameraComponent->PopCameraMode();

	if (Action->PushedCameraModes.Num() > 0)
	{
		Action->PushedCameraModes.Pop();
	}
}


void FPhaseBehavior_PlayCameraShake::Execute(ULuxAction* Action) const
{
	if (!Action || !ShakeClass)
	{
		return;
	}

	APawn* AvatarPawn = Cast<APawn>(Action->GetAvatarActor());
	if (!AvatarPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	PC->ClientStartCameraShake(ShakeClass, Scale);
}

void FPhaseBehavior_StartCooldown::Execute(ULuxAction* Action) const
{
	if (!Action || !Action->Cooldown) return;

	UActionSystemComponent* ASC = Action->GetActionSystemComponent();
	if (!ASC)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[StartCooldown] 액션 시스템 컴포넌트가 유효하지 않습니다."));
		return;
	}

	FActiveLuxAction* ActiveAction = ASC->FindActiveAction(Action->GetActiveHandle());
	if (!ActiveAction)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[StartCooldown] 활성 액션 정보를 찾을 수 없습니다."));
		return;
	}

	const FLuxActionSpecHandle SpecHandle = ActiveAction->Spec.Handle;
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[StartCooldown] 액션 스펙 핸들이 유효하지 않습니다."));
		return;
	}

	const FLuxActionSpec* ActionSpec = ASC->FindActionSpecFromHandle(SpecHandle);
	if (!ActionSpec)
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[StartCooldown] 핸들로부터 액션 스펙을 찾을 수 없습니다."));
		return;
	}

	// 쿨다운 이펙트 생성 및 적용
	FLuxEffectContextHandle Context = ASC->MakeEffectContext();
	Context.SetSourceAction(SpecHandle);

	FLuxEffectSpecHandle CooldownSpecHandle = ASC->MakeOutgoingSpec(Action->Cooldown, ActionSpec->Level, Context);
	if (CooldownSpecHandle.IsValid())
	{
		FActiveLuxEffectHandle Tmp; ASC->ApplyEffectSpecToSelf(CooldownSpecHandle, Tmp);
	}
	else
	{
		UE_LOG(LogLuxActionSystem, Error, TEXT("[StartCooldown] 쿨다운 이펙트 생성에 실패했습니다."));
	}
}