// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/TestPlayerState.h"
#include "ActionSystem/ActionSystemComponent.h"

#include "Character/LuxPawnData.h"
#include "Character/LuxPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "LuxLogChannels.h"


class AController;
class APlayerState;
class FLifetimeProperty;


ATestPlayerState::ATestPlayerState()
{
	NetUpdateFrequency = 100.0f;

    ActionSystemComponent = CreateDefaultSubobject<UActionSystemComponent>("ActionSystemComponent");
    ActionSystemComponent->SetIsReplicated(true);
}

void ATestPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
}

void ATestPlayerState::SetPawnData(const ULuxPawnData* InPawnData)
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

void ATestPlayerState::OnRep_PawnData()
{
}

UActionSystemComponent* ATestPlayerState::GetActionSystemComponent() const
{
	return ActionSystemComponent;
}