// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/LuxGameState.h"

#include "ActionSystem/ActionSystemComponent.h"
#include "Game/LuxPawnDataManagerComponent.h"
#include "Game/LuxPlayerState.h"

#include "Async/TaskGraphInterfaces.h"
#include "LuxLogChannels.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxGameState)

class APlayerState;
class FLifetimeProperty;

ALuxGameState::ALuxGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ActionSystemComponent = ObjectInitializer.CreateDefaultSubobject<UActionSystemComponent>(this, TEXT("ActionSystemComponent"));
	ActionSystemComponent->SetIsReplicated(true);

	PawnDataManager = ObjectInitializer.CreateDefaultSubobject<ULuxPawnDataManagerComponent>(this, TEXT("PawnDataManager"));
}

void ALuxGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void ALuxGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

}

void ALuxGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(ActionSystemComponent);
	ActionSystemComponent->InitActorInfo(/*Owner=*/ this, /*Avatar=*/ this);
}

void ALuxGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

}

void ALuxGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

void ALuxGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

}

void ALuxGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

}

void ALuxGameState::SeamlessTravelTransitionCheckpoint(bool bToTransitionMap)
{
	// ��Ȱ�� �÷��̾� �� ���� �����մϴ�.
	for (int32 i = PlayerArray.Num() - 1; i >= 0; i--)
	{
		APlayerState* PlayerState = PlayerArray[i];
		if (PlayerState && (PlayerState->IsABot() || PlayerState->IsInactive()))
		{
			RemovePlayerState(PlayerState);
		}
	}
}

UActionSystemComponent* ALuxGameState::GetActionSystemComponent() const
{
	return ActionSystemComponent;
}
