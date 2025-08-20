// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/LuxHeroComponent.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/LuxActionTagRelationshipMapping.h"

#include "Character/LuxPawnExtensionComponent.h"
#include "Character/LuxPawnData.h"
#include "Character/LuxCharacter.h"
#include "Character/LuxHeroCharacter.h"

#include "Camera/LuxCameraComponent.h"

#include "Game/LuxPlayerState.h"
#include "Game/LuxPlayerController.h"
#include "Game/TestPlayerState.h"

#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Logging/MessageLog.h"
#include "LuxLogChannels.h"
#include "LuxGameplayTags.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Input/LuxInputConfig.h"
#include "Input/LuxInputComponent.h"
#include "PlayerMappableInputConfig.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Animations/LuxAnimInstance.h"

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR



namespace LuxHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName ULuxHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName ULuxHeroComponent::NAME_ActorFeatureName("Hero");

ULuxHeroComponent::ULuxHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReadyToBindInputs = false;
	bIsMouseCursorVisible = false;
}

void ULuxHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogLux, Error, TEXT("[ULuxHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));
	}
	else
	{
		RegisterInitStateFeature();
	}
}

bool ULuxHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	// [Uninitialized -> Spawned] 유효한 Pawn이 존재한다면 상태 전환을 허용합니다.
	if (!CurrentState.IsValid() && DesiredState == LuxGameplayTags::InitState_Spawned)
	{
		if (Pawn) return true;
	}

	// [Spawned -> DataAvailable] PlayerState가 준비될 때까지 대기합니다.
	else if (CurrentState == LuxGameplayTags::InitState_Spawned && DesiredState == LuxGameplayTags::InitState_DataAvailable)
	{
		if (!GetPlayerState<ALuxPlayerState>())
			return false;

		// 서버 또는 Autonomous(클라이언트 자신)인 경우 Controller와 PlayerState의 소유 관계를 확인합니다.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();
			if (!Controller) return false;

			APlayerState* PlayerState = Controller->PlayerState;
			if (!PlayerState) return false;

			if (PlayerState->GetOwner() != Controller)
				return false;
		}

		// 로컬에서 제어되는 실제 플레이어의 경우 InputComponent와 LocalPlayer가 필요합니다.
		if (Pawn->IsLocallyControlled() && !Pawn->IsBotControlled())
		{
			APlayerController* LuxPC = GetController<APlayerController>();
			if (!Pawn->InputComponent || !LuxPC || !LuxPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}

	// [DataAvailable -> DataInitialized] 확장 컴포넌트의 초기화가 완료될 때까지 대기합니다.
	else if (CurrentState == LuxGameplayTags::InitState_DataAvailable && DesiredState == LuxGameplayTags::InitState_DataInitialized)
	{
		ALuxPlayerState* LuxPS = GetPlayerState<ALuxPlayerState>();
		return LuxPS && Manager->HasFeatureReachedInitState(Pawn, ULuxPawnExtensionComponent::NAME_ActorFeatureName, LuxGameplayTags::InitState_DataInitialized);
	}

	// [DataInitialized -> GameplayReady]
	else if (CurrentState == LuxGameplayTags::InitState_DataInitialized && DesiredState == LuxGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void ULuxHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == LuxGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		if (Pawn && Pawn->InputComponent)
		{
			bReadyToBindInputs = true;
			InitializePlayerInput(Pawn->InputComponent);
			
			// PawnData가 설정된 후 카메라 모드를 설정합니다.
			SetupDefaultCameraMode();
		}
	}
}

void ULuxHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// Ȯ�� ������Ʈ�� ���°� DataInitialized �� �� ���� �ʱ�ȭ ���·� ������ �õ��մϴ�.
	if (Params.FeatureName == ULuxPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == LuxGameplayTags::InitState_DataInitialized)
		{
			CheckDefaultInitialization();
		}
	}
}

void ULuxHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { LuxGameplayTags::InitState_Spawned, LuxGameplayTags::InitState_DataAvailable, LuxGameplayTags::InitState_DataInitialized, LuxGameplayTags::InitState_GameplayReady };
	ContinueInitStateChain(StateChain);
}

void ULuxHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	BindOnActorInitStateChanged(ULuxPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	ensure(TryToChangeInitState(LuxGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void ULuxHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void ULuxHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();
	SetupInputMappings(Pawn, Subsystem, PlayerInputComponent);

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void ULuxHeroComponent::SetupDefaultCameraMode()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	// HeroCharacter에서 카메라 컴포넌트를 가져옵니다.
	ALuxHeroCharacter* HeroCharacter = Cast<ALuxHeroCharacter>(Pawn);
	if (!HeroCharacter)
	{
		return;
	}

	ULuxCameraComponent* CameraComponent = HeroCharacter->GetCameraComponent();
	if (!CameraComponent)
	{
		return;
	}

	// PawnData에서 기본 카메라 모드를 가져옵니다.
	const ULuxPawnData* PawnData = nullptr;
	
	// 먼저 PawnExtensionComponent에서 시도
	const auto* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (PawnExtComp)
	{
		PawnData = PawnExtComp->GetPawnData<ULuxPawnData>();
	}
	
	// PawnExtensionComponent에서 찾지 못하면 PlayerState에서 시도
	if (!PawnData)
	{
		if (const ALuxPlayerState* PS = GetPlayerState<ALuxPlayerState>())
		{
			PawnData = PS->GetPawnData<ULuxPawnData>();
		}
	}

	if (PawnData && PawnData->DefaultCameraMode)
	{
		// 블루프린트 인스턴스를 사용합니다.
		UE_LOG(LogLux, Log, TEXT("HeroComponent: PawnData 가 유효하고 카메라 모드가 설정되어 있어 '%s' 카메라 모드를 사용합니다."), *PawnData->DefaultCameraMode->GetName());
		CameraComponent->PushCameraMode(PawnData->DefaultCameraMode);
	}
}

void ULuxHeroComponent::SetupInputMappings(const APawn* Pawn, UEnhancedInputLocalPlayerSubsystem* Subsystem, UInputComponent* PlayerInputComponent)
{
	const auto* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtComp)
	{
		return;
	}

	const auto* PawnData = PawnExtComp->GetPawnData<ULuxPawnData>();
	if (!PawnData)
	{
		return;
	}

	const ULuxInputConfig* InputConfig = PawnData->InputConfig;
	if (!InputConfig)
	{
		return;
	}

	for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
	{
		if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
		{
			FModifyContextOptions Options = {};
			Options.bIgnoreAllPressedKeysUntilRelease = true;
			Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
		}
	}

	ULuxInputComponent* LuxIC = Cast<ULuxInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(LuxIC, TEXT("Unexpected Input Component class! Change to ULuxInputComponent or subclass.")))
	{
		return;
	}

	// Add the key mappings that may have been set by the player
	LuxIC->AddInputMappings(InputConfig, Subsystem);

	// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
	// be triggered directly by these input actions Triggered events. 
	TArray<uint32> BindHandles;
	LuxIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_ActionInputTagPressed, &ThisClass::Input_ActionInputTagReleased, /*out*/ BindHandles);
	
	LuxIC->BindNativeAction(InputConfig, LuxGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
	LuxIC->BindNativeAction(InputConfig, LuxGameplayTags::InputTag_Jump, ETriggerEvent::Started, this, &ThisClass::Input_Jump, /*bLogIfNotFound=*/ false);
	LuxIC->BindNativeAction(InputConfig, LuxGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
	LuxIC->BindNativeAction(InputConfig, LuxGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
	LuxIC->BindNativeAction(InputConfig, LuxGameplayTags::InputTag_ToggleMouseCursor, ETriggerEvent::Started, this, &ThisClass::Input_ToggleMouseCursor, /*bLogIfNotFound=*/ false);

	if (ALuxPlayerController* PC = GetController<ALuxPlayerController>())
	{
		LuxIC->BindNativeAction(InputConfig, LuxGameplayTags::InputTag_Debug_ToggleActionSystem, ETriggerEvent::Started, PC, &ALuxPlayerController::ToggleActionSystemDebug, /*bLogIfNotFound=*/ false);
	}
}

bool ULuxHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void ULuxHeroComponent::Input_ActionInputTagPressed(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	const ULuxPawnExtensionComponent* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtComp) return;

	UActionSystemComponent* LuxASC = PawnExtComp->GetActionSystemComponent();
	if (!LuxASC) return;

	LuxASC->ActionInputTagPressed(InputTag);
}

void ULuxHeroComponent::Input_ActionInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	const ULuxPawnExtensionComponent* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtComp) return;

	UActionSystemComponent* LuxASC = PawnExtComp->GetActionSystemComponent();
	if (!LuxASC) return;

	LuxASC->ActionInputTagReleased(InputTag);
}

void ULuxHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	ACharacter* Character = GetPawn<ACharacter>();
	AController* Controller = Character ? Character->GetController() : nullptr;

	/*IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(Character);
	if (!ASCInterface) return;

	UActionSystemComponent* LuxASC = ASCInterface->GetActionSystemComponent();
	if (!LuxASC) return;

	if (LuxASC->HasTag(LuxGameplayTags::State_Block_Movement))
	{
		return;
	}*/

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Character->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Character->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void ULuxHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	ACharacter* Character = GetPawn<ACharacter>();
	AController* Controller = Character ? Character->GetController() : nullptr;

	const ULuxPawnExtensionComponent* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(Character);
	if (!PawnExtComp) return;

	UActionSystemComponent* LuxASC = PawnExtComp->GetActionSystemComponent();
	if (!LuxASC) return;

	if (LuxASC->HasTag(LuxGameplayTags::State_Block_Rotation))
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Character->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Character->AddControllerPitchInput(Value.Y);
	}
}

void ULuxHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * LuxHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * LuxHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

void ULuxHeroComponent::Input_Jump()
{
	if (ACharacter* Character = GetPawn<ACharacter>())
	{
		Character->Jump();
	}
}

void ULuxHeroComponent::Input_ToggleMouseCursor()
{
	// Alt 키 입력 시 마우스 커서 표시 상태를 토글합니다.
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	APlayerController* PC = GetController<APlayerController>();
	if (!PC) return;

	// 마우스 커서 상태 토글
	bIsMouseCursorVisible = !bIsMouseCursorVisible;
	
	// PlayerController에 마우스 커서 표시 상태 적용
	PC->bShowMouseCursor = bIsMouseCursorVisible;
	
	// 입력 모드 설정
	if (bIsMouseCursorVisible)
	{
		// 마우스 커서가 표시될 때: UI 입력 가능, 게임 입력 유지
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		
		UE_LOG(LogLux, Log, TEXT("마우스 커서가 표시되었습니다."));
	}
	else
	{
		// 마우스 커서가 숨겨질 때: 게임 입력만 가능
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		
		UE_LOG(LogLux, Log, TEXT("마우스 커서가 숨겨졌습니다."));
	}
}
