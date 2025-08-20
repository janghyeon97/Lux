
#include "Character/LuxCharacter.h"

//#include "Character/HealthComponent.h"
#include "Character/LuxPawnExtensionComponent.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Attributes/MovementSet.h"

#include "Game/LuxPlayerState.h"

#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

#include "Animations/LuxAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/LuxCharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxCharacter)

class AController;
class APlayerState;
class FLifetimeProperty;

FName ALuxCharacter::NAME_LuxCharacterCollisionProfile_Capsule(TEXT("LuxPawnCapsule"));
FName ALuxCharacter::NAME_LuxCharacterCollisionProfile_Mesh(TEXT("LuxPawnMesh"));

FString ALuxCharacter::ClientServerStatus = TEXT("Client");

ALuxCharacter::ALuxCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<ULuxCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 네트워크 복제 설정
	bReplicates = true;
	SetReplicates(true);

	NetCullDistanceSquared = 900000000.0f;
	NetUpdateFrequency = 20.f;  // 네트워크 업데이트 빈도: 20Hz

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, -95.0f));
	MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));

	PawnExtComponent = CreateDefaultSubobject<ULuxPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnActionSystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ALuxCharacter::OnActionSystemInitialized));
	PawnExtComponent->OnActionSystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ALuxCharacter::OnActionSystemUninitialized));

	/*HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ALuxCharacter::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ALuxCharacter::OnDeathFinished);*/

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	bIsActionControlledMovement = false;
	ClientServerStatus = GetClientServerContextString(this);
}

UActionSystemComponent* ALuxCharacter::GetActionSystemComponent() const
{
	return PawnExtComponent ? PawnExtComponent->GetActionSystemComponent() : nullptr;
}

void ALuxCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();

}

void ALuxCharacter::BeginPlay()
{
	Super::BeginPlay();


}

void ALuxCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

}

void ALuxCharacter::Reset()
{
	DisableMovementAndCollision();
	K2_OnReset();
	UninitAndDestroy();
}

void ALuxCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MyTeamID);
	DOREPLIFETIME_CONDITION(ThisClass, bIsActionControlledMovement, COND_None);
}

void ALuxCharacter::OnActionSystemInitialized()
{
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC)
	{
		return;
	}

	UMovementSet* MovementSet = ASC->GetMutableAttributeSet<UMovementSet>();
	if (MovementSet)
	{
		MovementSet->OnMoveSpeedChanged.AddDynamic(this, &ALuxCharacter::OnMoveSpeedChanged);
		GetCharacterMovement()->MaxWalkSpeed = MovementSet->GetMoveSpeed();
	}
}

void ALuxCharacter::OnActionSystemUninitialized()
{
	UActionSystemComponent* ASC = GetActionSystemComponent();
	if (!ASC)
	{
		return;
	}

	UMovementSet* MovementSet = ASC->GetMutableAttributeSet<UMovementSet>();
	if (MovementSet)
	{
		MovementSet->OnMoveSpeedChanged.RemoveDynamic(this, &ALuxCharacter::OnMoveSpeedChanged);
	}
}

void ALuxCharacter::NotifyControllerChanged()
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();

	Super::NotifyControllerChanged();

	// Update our team ID based on the controller
	if (HasAuthority() && (Controller != nullptr))
	{
		if (IGenericTeamAgentInterface* ControllerWithTeam = Cast<IGenericTeamAgentInterface>(Controller))
		{
			MyTeamID = ControllerWithTeam->GetGenericTeamId();
		}
	}
}

void ALuxCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
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

FGenericTeamId ALuxCharacter::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnTeamIndexChangedDelegate* ALuxCharacter::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ALuxCharacter::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamID = MyTeamID;
	Super::PossessedBy(NewController);

	// Pawn Extension Component에 컨트롤러 변경을 알립니다.
	PawnExtComponent->HandleControllerChanged();

	ILuxTeamAgentInterface* TeamAgent = Cast<ILuxTeamAgentInterface>(NewController);
	if (TeamAgent)
	{
		MyTeamID = TeamAgent->GetGenericTeamId();
		TeamAgent->GetTeamChangedDelegateChecked().AddDynamic(this, &ALuxCharacter::OnControllerChangedTeam);
	}

	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALuxCharacter::UnPossessed()
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

void ALuxCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void ALuxCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtComponent->HandlePlayerStateReplicated();
}

void ALuxCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void ALuxCharacter::OnDeathStarted(AActor* OwningActor)
{
	DisableMovementAndCollision();
}

void ALuxCharacter::OnDeathFinished(AActor* OwningActor)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ALuxCharacter::DestroyDueToDeath);
}

void ALuxCharacter::OnMoveSpeedChanged(float OldValue, float NewValue)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = NewValue;
	}
}

void ALuxCharacter::DisableMovementAndCollision()
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UCharacterMovementComponent* MoveComp = CastChecked<UCharacterMovementComponent>(GetCharacterMovement());
	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();
}

void ALuxCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();
	UninitAndDestroy();
}

void ALuxCharacter::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	SetActorHiddenInGame(true);
}

void ALuxCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALuxCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	// 새 팀 ID로 갱신하고 팀 변경 이벤트를 브로드캐스트합니다.
	const FGenericTeamId MyOldTeamID = MyTeamID;
	MyTeamID = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, MyOldTeamID, MyTeamID);
}

void ALuxCharacter::SetActionControlledMovement(bool bIsActionControlled)
{
	if (HasAuthority())
	{
		bIsActionControlledMovement = bIsActionControlled;
	}
}
