// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LuxHeroCharacter.h"
#include "Character/LuxHeroComponent.h"
#include "Character/LuxPawnData.h"
#include "Character/LuxPawnExtensionComponent.h"
#include "Targeting/TargetingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Game/LuxPlayerState.h"
#include "GameFramework/SpringArmComponent.h"

#include "Camera/LuxCameraComponent.h"
#include "LuxLogChannels.h"

ALuxHeroCharacter::ALuxHeroCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 네트워크 복제 설정
	bReplicates = true;
	SetReplicates(true);

	HeroComponent = CreateDefaultSubobject<ULuxHeroComponent>(TEXT("HeroComponent"));
	check(HeroComponent);

	NetUpdateFrequency = 20.f;  // 네트워크 업데이트 빈도: 20Hz

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionProfileName(TEXT("Player"));

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	check(SpringArmComponent);
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 500.f;
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->bDoCollisionTest = true;
	SpringArmComponent->bInheritPitch = true;
	SpringArmComponent->bInheritYaw = true;
	SpringArmComponent->bInheritRoll = false;

	CameraComponent = CreateDefaultSubobject<ULuxCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->PostProcessSettings.bOverride_MotionBlurAmount = true;
	CameraComponent->PostProcessSettings.MotionBlurAmount = 0.0f;

	/*FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	check(FollowCamera);
	FollowCamera->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	FollowCamera->SetRelativeLocation(FVector(0.f, 80.f, 100.f));
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	FollowCamera->PostProcessSettings.MotionBlurAmount = 0.0f;*/

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	check(MoveComp);
	MoveComp->GravityScale = 1.0f;
	MoveComp->MaxAcceleration = 2400.0f;
	MoveComp->BrakingFrictionFactor = 1.0f;
	MoveComp->BrakingFriction = 6.0f;
	MoveComp->GroundFriction = 8.0f;
	MoveComp->BrakingDecelerationWalking = 1400.0f;
	MoveComp->bUseControllerDesiredRotation = false;
	MoveComp->bOrientRotationToMovement = false;
	MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	MoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	MoveComp->bCanWalkOffLedgesWhenCrouching = true;
	MoveComp->SetCrouchedHalfHeight(65.0f);

	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	check(TargetingComponent);
}

UActionSystemComponent* ALuxHeroCharacter::GetActionSystemComponent() const
{
	if (ALuxPlayerState* PS = GetPlayerState<ALuxPlayerState>())
	{
		return PS->GetActionSystemComponent();
	}

	return nullptr;
}

void ALuxHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 카메라 모드 설정은 HeroComponent에서 처리합니다.
	// PawnData가 설정된 후 DataInitialized 상태에서 자동으로 설정됩니다.
}



const ULuxPawnData* ALuxHeroCharacter::GetPawnData() const
{
	if (const ALuxPlayerState* PS = GetPlayerState<ALuxPlayerState>())
	{
		return PS->GetPawnData<ULuxPawnData>();
	}

	if (PawnExtComponent)
	{
		return PawnExtComponent->GetPawnData<ULuxPawnData>();
	}

	return nullptr;
}

void ALuxHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
}

void ALuxHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALuxHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ALuxHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

}

void ALuxHeroCharacter::UnPossessed()
{
	Super::UnPossessed();

}

void ALuxHeroCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
}

void ALuxHeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	// 카메라 모드 설정은 HeroComponent에서 처리합니다.
}

void ALuxHeroCharacter::OnActionSystemInitialized()
{
	Super::OnActionSystemInitialized();
}


void ALuxHeroCharacter::OnActionSystemUninitialized()
{
	Super::OnActionSystemUninitialized();
}

