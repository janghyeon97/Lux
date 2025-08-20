// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LuxAICharacter.h"

// Engine
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

// Lux - Character
#include "Character/LuxPawnData.h"
#include "Character/LuxPawnExtensionComponent.h"

// Lux - Action System
#include "ActionSystem/ActionSystemComponent.h"

// Lux - UI
#include "UI/VitalsWidget.h"
#include "UI/AttributeBarBase.h"

// Lux - Game
#include "Game/LuxPawnDataManagerComponent.h"
#include "Game/LuxGameState.h"

// Lux - System
#include "System/LuxAssetManager.h"
#include "LuxLogChannels.h"

ALuxAICharacter::ALuxAICharacter()
{
	bReplicates = true;
	SetReplicates(true);

	ActionSystemComponent = CreateDefaultSubobject<UActionSystemComponent>(TEXT("ActionSystemComponent"));
	ActionSystemComponent->SetIsReplicated(true);

	// VitalsWidget을 위한 WidgetComponent 생성
	VitalsWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("VitalsWidgetComponent"));
	VitalsWidgetComponent->SetupAttachment(GetRootComponent());
	VitalsWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	VitalsWidgetComponent->SetDrawAtDesiredSize(true);
	VitalsWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
	VitalsWidgetComponent->SetVisibility(false);

	NetUpdateFrequency = 20.f;
}

void ALuxAICharacter::BeginPlay()
{
	Super::BeginPlay();

}

void ALuxAICharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(HasAuthority() == false)
	{
		return;
	}

	UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: PostInitializeComponents 시작 - PawnData 초기화 중..."));

	// PawnExtComponent 찾기
	ULuxPawnExtensionComponent* PawnExt = ULuxPawnExtensionComponent::FindPawnExtensionComponent(this);
	if (!PawnExt)
	{
		UE_LOG(LogLux, Error, TEXT("ALuxAICharacter: PostInitializeComponents에서 PawnExtComponent를 찾을 수 없습니다."));
		return;
	}

	// PawnData가 이미 설정되어 있는지 확인
	if (PawnExt->GetPawnData<ULuxPawnData>())
	{
		UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: PostInitializeComponents에서 PawnData가 이미 설정되어 있습니다."));
		return;
	}

	// 1. AIPawnDataId가 설정되어 있을 경우 우선 로드
	if (AIPawnDataId.IsValid())
	{
		UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: AIPawnDataId를 통해 PawnData를 로드합니다: %s"), *AIPawnDataId.ToString());
		
		if (TryLoadPawnDataById(AIPawnDataId))
		{
			UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: AIPawnDataId를 통한 PawnData 로드 성공"));
			return;
		}
		else
		{
			UE_LOG(LogLux, Warning, TEXT("ALuxAICharacter: AIPawnDataId를 통한 PawnData 로드 실패"));
		}
	}

	// 2. AIPawnDataPath가 설정되어 있을 경우 직접 로드
	if (!AIPawnDataPath.IsNull())
	{
		UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: AIPawnDataPath를 통해 PawnData를 로드합니다: %s"), *AIPawnDataPath.ToString());
		
		// 직접 LoadSynchronous 사용
		if (ULuxPawnData* AIPawnData = AIPawnDataPath.LoadSynchronous())
		{
			UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: 직접 로드로 PawnData 설정 성공: %s"), *AIPawnData->GetName());
			PawnExt->SetPawnData(AIPawnData);
			return;
		}
		else
		{
			UE_LOG(LogLux, Warning, TEXT("ALuxAICharacter: AIPawnDataPath 직접 로드 실패: %s"), *AIPawnDataPath.ToString());
		}
	}

	// 3. 모든 방법이 실패한 경우 초기화 실패
	UE_LOG(LogLux, Error, TEXT("ALuxAICharacter: PostInitializeComponents에서 PawnData 초기화 실패. AIPawnDataId 또는 AIPawnDataPath를 확인해주세요."));
}

bool ALuxAICharacter::TryLoadPawnDataById(const FPrimaryAssetId& PawnDataId)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	
	ALuxGameState* GameState = Cast<ALuxGameState>(UGameplayStatics::GetGameState(World));
	if (!GameState)
	{
		UE_LOG(LogLux, Error, TEXT("ALuxAICharacter: TryLoadPawnDataById에서 GameState를 찾을 수 없습니다."));
		return false;
	}
	
	ULuxPawnDataManagerComponent* PawnDataManager = GameState->GetPawnDataManagerComponent();
	if (!PawnDataManager)
	{
		UE_LOG(LogLux, Error, TEXT("ALuxAICharacter: TryLoadPawnDataById에서 PawnDataManager를 찾을 수 없습니다."));
		return false;
	}
	
	// PawnExtComponent 찾기
	ULuxPawnExtensionComponent* PawnExt = ULuxPawnExtensionComponent::FindPawnExtensionComponent(this);
	if (!PawnExt)
	{
		UE_LOG(LogLux, Error, TEXT("ALuxAICharacter: TryLoadPawnDataById에서 PawnExtComponent를 찾을 수 없습니다."));
		return false;
	}
	
	// 이미 로드된 경우 즉시 설정
	if (PawnDataManager->IsPawnDataLoaded(PawnDataId))
	{
		const ULuxPawnData* AIPawnData = PawnDataManager->GetPawnDataChecked(PawnDataId);
		UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: PawnData ID를 사용합니다: %s"), *AIPawnData->GetName());
		PawnExt->SetPawnData(AIPawnData);
		return true;
	}
	
	// 로드되지 않은 경우 로드 요청
	UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: PawnData 로드를 요청합니다: %s"), *PawnDataId.ToString());
	PawnDataManager->CallOrRegister_OnPawnDataLoaded(PawnDataId, 
		FOnPawnDataLoaded::FDelegate::CreateUObject(this, &ALuxAICharacter::OnDefaultPawnDataLoaded));
	
	// 로드 시작
	PawnDataManager->StartPawnDataLoad(PawnDataId);
	return true;
}


void ALuxAICharacter::OnDefaultPawnDataLoaded(const ULuxPawnData* PawnData)
{
	if (!PawnData)
	{
		UE_LOG(LogLux, Warning, TEXT("ALuxAICharacter: 콜백에서 null PawnData를 받았습니다."));
		return;
	}
	
	ULuxPawnExtensionComponent* PawnExt = ULuxPawnExtensionComponent::FindPawnExtensionComponent(this);
	if (!PawnExt)
	{
		UE_LOG(LogLux, Warning, TEXT("ALuxAICharacter: 콜백에서 PawnExtComponent를 찾을 수 없습니다."));
		return;
	}
	
	UE_LOG(LogLux, Log, TEXT("ALuxAICharacter: 기본 AI PawnData가 성공적으로 로드되었습니다: %s"), *PawnData->GetName());
	PawnExt->SetPawnData(PawnData);
}

UActionSystemComponent* ALuxAICharacter::GetActionSystemComponent() const
{
	return ActionSystemComponent;
}

void ALuxAICharacter::OnActionSystemInitialized()
{
	Super::OnActionSystemInitialized();

	if(HasAuthority())
	{
		return;
	}

	if (!VitalsWidgetComponent)
	{
		UE_LOG(LogLux, Warning, TEXT("ALuxAICharacter: VitalsWidgetComponent가 비어있습니다."));
		return;
	}

	if (!VitalsWidgetClass)
	{
		UE_LOG(LogLux, Warning, TEXT("ALuxAICharacter: VitalsWidgetClass가 비어있습니다."));
		return;
	}

	UVitalsWidget* VitalsWidget = CreateWidget<UVitalsWidget>(GetWorld(), VitalsWidgetClass);
	if (!VitalsWidget)
	{
		UE_LOG(LogLux, Error, TEXT("ALuxAICharacter: VitalsWidget 인스턴스를 생성하지 못했습니다."));
		return;
	}

	VitalsWidgetComponent->SetWidget(VitalsWidget);
	VitalsWidget->InitializeWidget(ActionSystemComponent);
	
	// Exp 바 숨기기
	HideExperienceBar(VitalsWidget);
	
	VitalsWidgetComponent->SetVisibility(true);

	UE_LOG(LogLux, Log, TEXT("[%s][%s]: VitalsWidgetComponent가 생성되었습니다"), *ClientServerStatus, ANSI_TO_TCHAR(__FUNCTION__));
}

void ALuxAICharacter::HideExperienceBar(UVitalsWidget* VitalsWidget)
{
	if (VitalsWidget)
	{
		// ExperienceBar를 찾아서 숨기기
		UAttributeBarBase* ExperienceBar = VitalsWidget->ExperienceBar;
		if (ExperienceBar)	
		{
			ExperienceBar->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
