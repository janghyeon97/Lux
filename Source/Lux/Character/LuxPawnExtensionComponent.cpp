// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LuxPawnExtensionComponent.h"

#include "Game/LuxPlayerState.h"
#include "Game/TestPlayerState.h"
#include "Character/LuxHeroComponent.h" 
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/ActionSystemInterface.h"

#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "System/LuxAssetManager.h" 
#include "ActionSystem/LuxActionSet.h"
#include "LuxPawnData.h"
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LuxPawnExtensionComponent)

class FLifetimeProperty;
class UActorComponent;

const FName ULuxPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

ULuxPawnExtensionComponent::ULuxPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	PawnData = nullptr;
	ActionSystemComponent = nullptr;
}

void ULuxPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULuxPawnExtensionComponent, PawnData);
}


/*
 * IGameFrameworkInitStateInterface의 일부로, OnRegister에서는 아래 작업을 수행해야 합니다:
 * 1. 컴포넌트를 InitState 시스템에 등록합니다.
 * 2. 소유 액터가 Pawn인지 확인합니다.
 * 3. 이 컴포넌트가 Pawn에 하나만 존재하는지 확인합니다.
 */
void ULuxPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	const APawn* Pawn = GetPawn<APawn>();
	ensureAlwaysMsgf((Pawn != nullptr), TEXT("LuxPawnExtensionComponent on [%s] can only be added to Pawn actors."), *GetNameSafe(GetOwner()));

	TArray<UActorComponent*> PawnExtensionComponents;
	Pawn->GetComponents(ULuxPawnExtensionComponent::StaticClass(), PawnExtensionComponents);
	ensureAlwaysMsgf((PawnExtensionComponents.Num() == 1), TEXT("Only one LuxPawnExtensionComponent should exist on [%s]."), *GetNameSafe(GetOwner()));

	RegisterInitStateFeature();
}

void ULuxPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 모든 feature들의 상태 변경을 수신 대기합니다.
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);

	// 생성 완료를 알려 초기화 프로세스를 진행합니다.
	ensure(TryToChangeInitState(LuxGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}


void ULuxPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeActionSystem();
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}


/*
 * GameFramework 초기화 과정의 일부로, CurrentState에서 DesiredState로의 전환이 가능한지 판단합니다.
 * false 반환 시 해당 초기화 단계가 보류되도록 만들 수 있습니다.
 */
bool ULuxPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);
	APawn* Pawn = GetPawn<APawn>();

	// 유효한 Pawn이 있어야 InitState_Spawned 단계로 진입
	if (!CurrentState.IsValid() && DesiredState == LuxGameplayTags::InitState_Spawned)
	{
		if (Pawn) return true;
	}

	if (CurrentState == LuxGameplayTags::InitState_Spawned && DesiredState == LuxGameplayTags::InitState_DataAvailable)
	{
		// PawnData가 있어야 DataAvailable 단계로 진입
		if (!PawnData)
		{
			return false;
		}

		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		// 서버 권한이 있거나 로컬에서 제어되는 경우, 컨트롤러가 있어야 함
		if (bHasAuthority || bIsLocallyControlled)
		{
			if (!GetController<AController>())
			{
				return false;
			}
		}

		return true;
	}

	// 모든 기능이 DataAvailable 상태에 도달하면 DataInitialized로 전환
	if (CurrentState == LuxGameplayTags::InitState_DataAvailable && DesiredState == LuxGameplayTags::InitState_DataInitialized)
	{
		return Manager->HaveAllFeaturesReachedInitState(Pawn, LuxGameplayTags::InitState_DataAvailable);
	}

	// 데이터 초기화 완료 후 GameplayReady로 진입
	if (CurrentState == LuxGameplayTags::InitState_DataInitialized && DesiredState == LuxGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}


/*
 * CanChangeInitState가 true를 반환하여 DesiredState 단계로의 전환이 허용될 때 호출됩니다.
 * 이곳에서 DesiredState에 대한 커스텀 초기화 로직을 수행할 수 있습니다.
 */
void ULuxPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == LuxGameplayTags::InitState_DataInitialized)
	{
		// DataInitialized 단계 전환 시, 이미 완료된 다른 컴포넌트들의 이벤트를 처리합니다.
		InitializeActionSystem();
	}
}


/*
 * 소유 Actor의 다른 초기화 기능(feature)의 상태가 변경될 때 호출됩니다.
 * Params로 전달된 정보를 바탕으로 CheckDefaultInitialization을 재호출하거나 추가 로직을 수행할 수 있습니다.
 */
void ULuxPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// 다른 기능이 DataAvailable 단계에 도달하면, DataInitialized로 전환할 수 있는지 확인합니다.
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		if (Params.FeatureState == LuxGameplayTags::InitState_DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
}


/*
 * 정의된 InitState 체인({Spawned, DataAvailable, DataInitialized, GameplayReady} 등)을
 * 현재 상태에서 가능한 한 멀리까지 전환을 시도합니다.
 */
void ULuxPawnExtensionComponent::CheckDefaultInitialization()
{
	// 상태 진행 전, 구현 클래스들의 초기화를 우선 처리합니다.
	CheckDefaultInitializationForImplementers();

	static const TArray<FGameplayTag> StateChain = {
		LuxGameplayTags::InitState_Spawned,
		LuxGameplayTags::InitState_DataAvailable,
		LuxGameplayTags::InitState_DataInitialized,
		LuxGameplayTags::InitState_GameplayReady
	};

	// Spawned 단계부터 DataAvailable, DataInitialized, GameplayReady까지 순차적으로 상태를 전환합니다.
	ContinueInitStateChain(StateChain);
}

void ULuxPawnExtensionComponent::SetPawnData(const ULuxPawnData* InPawnData)
{
	check(InPawnData);
	APawn* Pawn = GetPawnChecked<APawn>();
	if (Pawn->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogLux, Error, TEXT("Trying to set PawnData [%s] on pawn [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(Pawn), *GetNameSafe(PawnData));
		return;
	}

	PawnData = InPawnData;

	if (PawnData)
	{
		ULuxAssetManager& AssetManager = ULuxAssetManager::Get();
		for (const TSoftClassPtr<AActor>& ActorClassPtr : PawnData->PreloadedActorClasses)
		{
			// AssetManager를 통해 미리 클래스를 로드하여 메모리에 올려둡니다.
			AssetManager.GetSubclass(ActorClassPtr);
		}
	}

	Pawn->ForceNetUpdate();
	CheckDefaultInitialization();
}

void ULuxPawnExtensionComponent::InitializeActionSystem()
{
	APawn* Pawn = GetPawnChecked<APawn>();
	AActor* OwnerActor = Pawn; // 기본값은 폰 자신입니다.
	AActor* AvatarActor = Pawn; // Avatar는 항상 폰 자신입니다.

	// PlayerState를 우선적으로 사용 (HeroComponent가 있든 없든)
	ALuxPlayerState* LuxPs = Pawn->GetPlayerState<ALuxPlayerState>();
	if (LuxPs)
	{
		OwnerActor = LuxPs;
		UE_LOG(LogLux, Log, TEXT("InitializeActionSystem: Pawn '%s'에서 PlayerState '%s'를 Owner로 사용"), *Pawn->GetName(), *LuxPs->GetName());
	}
	else
	{
		UE_LOG(LogLux, Log, TEXT("InitializeActionSystem: Pawn '%s'에서 PlayerState가 없어 Pawn 자신을 Owner로 사용"), *Pawn->GetName());
	}

	// OwnerActor가 유효한지 체크
	if (!OwnerActor)
	{
		UE_LOG(LogLux, Error, TEXT("PawnExtension: OwnerActor가 nullptr입니다"));
		return;
	}

	IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(OwnerActor);
	if (!ASCInterface)
	{
		UE_LOG(LogLux, Error, TEXT("PawnExtension: OwnerActor [%s] does not implement IActionSystemInterface"), *GetNameSafe(OwnerActor));
		return;
	}

	UActionSystemComponent* ASC = ASCInterface->GetActionSystemComponent();
	if (!ASC)
	{
		UE_LOG(LogLux, Error, TEXT("PawnExtension: GetActionSystemComponent returned NULL on [%s]"), *GetNameSafe(OwnerActor));
		return;
	}

	if (!PawnData)
	{
		UE_LOG(LogLux, Error, TEXT("PawnExtension: Cannot find PawnData on [%s]"), *GetNameSafe(Pawn));
		return;
	}

	if (ASC->bCharacterActionsGiven)
		return; // 이미 ActionSystem이 초기화된 경우, 중복 초기화를 방지합니다.

	ActionSystemComponent = ASC;
	ActionSystemComponent->InitActorInfo(OwnerActor, AvatarActor);
	ActionSystemComponent->SetTagRelationshipMapping(PawnData->TagRelationshipMapping);

	for (const ULuxActionSet* ActionSet : PawnData->ActionSets)
	{
		if (ActionSet)
		{
			ActionSet->GrantToActionSystem(ASC, nullptr, OwnerActor);
		}
	}

	for (const FLuxActionSpec& Spec : ActionSystemComponent->GetActionSpecs())
	{
		const ULuxAction* Action = Spec.Action;
		if (Action && Action->GetActivationPolicy() == ELuxActionActivationPolicy::OnSpawn)
		{
			ActionSystemComponent->TryExecuteAction(Spec.Handle);
		}
	}

	UE_LOG(LogLux, Warning, TEXT("[%s][%s] OwnerActor: %s, AvatarActor: %s, 액션 시스템이 초기화 완료되었습니다. OnActionSystemInitialized 브로드캐스트 합니다."), *GetClientServerContextString(AvatarActor), ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(OwnerActor), *GetNameSafe(AvatarActor));
	OnActionSystemInitialized.Broadcast();
}


void ULuxPawnExtensionComponent::UninitializeActionSystem()
{
	if (!ActionSystemComponent)
	{
		return;
	}

	if (ActionSystemComponent->GetAvatarActor() == GetOwner())
	{
		// 모든 Ability를 취소하되, 종료 시에도 유지되어야 할 효과(예: 죽음 애니메이션)를 가진 'SurvivesDeath' 태그가 붙은 Ability는 제외하고 처리합니다.
		FGameplayTagContainer IgnoreTags;
		IgnoreTags.AddTag(LuxGameplayTags::Action_Behavior_SurvivesDeath);

		ActionSystemComponent->CancelActions(nullptr, &IgnoreTags);
		ActionSystemComponent->ClearActionInput();

		if (ActionSystemComponent->GetOwnerActor() != nullptr)
		{
			ActionSystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			ActionSystemComponent->ClearActorInfo();
		}

		OnActionSystemUninitialized.Broadcast();
	}

	ActionSystemComponent = nullptr;
}

void ULuxPawnExtensionComponent::HandleControllerChanged()
{
	if (ActionSystemComponent && (ActionSystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()))
	{
		if (ActionSystemComponent->GetOwnerActor() == nullptr)
		{
			UninitializeActionSystem();
		}
		else
		{
			//ActionSystemComponent->RefreshAbilityActorInfo();
		}
	}

	CheckDefaultInitialization();
}

void ULuxPawnExtensionComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void ULuxPawnExtensionComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}

void ULuxPawnExtensionComponent::OnRep_PawnData()
{
	CheckDefaultInitialization();
}

void ULuxPawnExtensionComponent::OnActionSystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnActionSystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnActionSystemInitialized.Add(Delegate);
	}

	if (ActionSystemComponent)
	{
		Delegate.Execute();
	}
}

void ULuxPawnExtensionComponent::OnActionSystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnActionSystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnActionSystemUninitialized.Add(Delegate);
	}
}

