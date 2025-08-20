// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LuxPawn.h"
#include "Character/LuxPawnExtensionComponent.h"
#include "ActionSystem/ActionSystemComponent.h"

#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"

#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Perception/PawnSensingComponent.h"

ALuxPawn::ALuxPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 네트워크 복제 설정
	bReplicates = true;
	SetReplicates(true);

	PawnExtComponent = CreateDefaultSubobject<ULuxPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnActionSystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnActionSystemInitialized));
	PawnExtComponent->OnActionSystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnActionSystemUninitialized));

	NetUpdateFrequency = 20.f;  // 네트워크 업데이트 빈도: 20Hz

	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
}

void ALuxPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void ALuxPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

}

void ALuxPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALuxPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void ALuxPawn::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamID = MyTeamID;
	Super::PossessedBy(NewController);

	// Pawn Extension Component에 컨트롤러 변경을 알립니다.
	PawnExtComponent->HandleControllerChanged();

	ILuxTeamAgentInterface* TeamAgent = Cast<ILuxTeamAgentInterface>(NewController);
	if (TeamAgent)
	{
		MyTeamID = TeamAgent->GetGenericTeamId();
		TeamAgent->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
	}

	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALuxPawn::UnPossessed()
{
	AController* const OldController = Controller;

	const FGenericTeamId OldTeamID = MyTeamID;
	if (ILuxTeamAgentInterface* TeamAgent = Cast<ILuxTeamAgentInterface>(OldController))
	{
		MyTeamID = TeamAgent->GetGenericTeamId();
		TeamAgent->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	Super::UnPossessed();

	// Pawn Extension Component에 컨트롤러 변경을 알립니다.
	PawnExtComponent->HandleControllerChanged();

	// UnPossessed 이후 새 팀 ID를 결정합니다.
	MyTeamID = DetermineNewTeamAfterPossessionEnds(OldTeamID);
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALuxPawn::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void ALuxPawn::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (GetController())
	{
		UE_LOG(LogLuxTeams, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetPathNameSafe(this));
		return;
	}

	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
	}
	else
	{
		UE_LOG(LogLuxTeams, Error, TEXT("You can't set the team ID on a non-authoritative character (%s); it's driven by the associated controller"), *GetPathNameSafe(this));
	}
}

FGenericTeamId ALuxPawn::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnTeamIndexChangedDelegate* ALuxPawn::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ALuxPawn::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALuxPawn::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	// 새 팀 ID로 갱신하고 팀 변경 이벤트를 브로드캐스트합니다.
	const FGenericTeamId MyOldTeamID = MyTeamID;
	MyTeamID = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, MyOldTeamID, MyTeamID);
}

void ALuxPawn::OnActionSystemInitialized()
{
}

void ALuxPawn::OnActionSystemUninitialized()
{
}