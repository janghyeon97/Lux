// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/LuxPlayerState.h"
#include "Game/LuxGameMode.h"
#include "Game/LuxPlayerController.h"
#include "Game/LuxPawnDataManagerComponent.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/LuxActionSet.h"

#include "Character/LuxPawnData.h"
#include "Character/LuxPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "LuxLogChannels.h"

class AController;
class APlayerState;
class FLifetimeProperty;

const FName ALuxPlayerState::NAME_LuxAbilityReady("LuxAbilitiesReady");

ALuxPlayerState::ALuxPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PlayerConnectionType(ELuxPlayerConnectionType::Player)
{
	bReplicates = true;
	SetReplicates(true);

	ActionSystemComponent = CreateDefaultSubobject<UActionSystemComponent>(TEXT("ActionSystemComponent"));
	ActionSystemComponent->SetIsReplicated(true);

	// PlayerState의 NetUpdateFrequency는 캐릭터에 비해 높게 설정합니다.
	// 기본값으로는 지연 시간에 따라 액션 시스템에서 체력 확인 문제가 발생할 수 있습니다.
	// 100은 고속 연결에서는 꽤 높은 수치일 수 있으므로, 프로젝트의 필요에 맞게 조정하십시오.
	NetUpdateFrequency = 100.0f;

	MyTeamID = FGenericTeamId::NoTeam;
}

void ALuxPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerConnectionType, SharedParams);
}

void ALuxPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

}

void ALuxPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(ActionSystemComponent);
	ActionSystemComponent->InitActorInfo(this, GetPawn());

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_Client)
	{
		return;
	}
}

void ALuxPlayerState::SetPawnData(const ULuxPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogLux, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	ForceNetUpdate();
}

void ALuxPlayerState::SetPlayerConnectionType(ELuxPlayerConnectionType NewType)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PlayerConnectionType, this);
	PlayerConnectionType = NewType;
}

void ALuxPlayerState::Reset()
{
	Super::Reset();

}

void ALuxPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	if (ULuxPawnExtensionComponent* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		PawnExtComp->CheckDefaultInitialization();
	}
}

void ALuxPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

}

void ALuxPlayerState::OnDeactivated()
{
	bool bDestroyDeactivatedPlayerState = false;

	switch (GetPlayerConnectionType())
	{
	case ELuxPlayerConnectionType::Player:
	case ELuxPlayerConnectionType::InactivePlayer:
		bDestroyDeactivatedPlayerState = true;
		break;
	default:
		bDestroyDeactivatedPlayerState = true;
		break;
	}

	SetPlayerConnectionType(ELuxPlayerConnectionType::InactivePlayer);

	if (bDestroyDeactivatedPlayerState)
	{
		Destroy();
	}
}

void ALuxPlayerState::OnReactivated()
{
	if (GetPlayerConnectionType() == ELuxPlayerConnectionType::InactivePlayer)
	{
		SetPlayerConnectionType(ELuxPlayerConnectionType::Player);
	}
}

void ALuxPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogLuxTeams, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}
}

FGenericTeamId ALuxPlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnTeamIndexChangedDelegate* ALuxPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

ALuxPlayerController* ALuxPlayerState::GetLuxPlayerController() const
{
	return Cast<ALuxPlayerController>(GetOwningController());
}

UActionSystemComponent* ALuxPlayerState::GetActionSystemComponent() const
{
	return ActionSystemComponent;
}

void ALuxPlayerState::OnRep_PawnData()
{
}

void ALuxPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}
