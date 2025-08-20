// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/LuxGameMode.h"
#include "Game/LuxGameState.h"
#include "Game/LuxPlayerState.h"
#include "Game/TestPlayerState.h"
#include "Game/LuxGameInstance.h"
#include "Game/LuxPlayerController.h"
#include "Game/LuxPawnDataManagerComponent.h"
#include "Character/LuxCharacter.h"
#include "Character/LuxPawnData.h"
#include "Character/LuxPawnExtensionComponent.h"

#include "UI/LuxHUD.h"
#include "System/LuxAssetManager.h"
#include "LuxLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ALuxGameMode::ALuxGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = ALuxGameState::StaticClass();
	PlayerControllerClass = ALuxPlayerController::StaticClass();
	PlayerStateClass = ALuxPlayerState::StaticClass();
	DefaultPawnClass = ALuxCharacter::StaticClass();
	HUDClass = ALuxHUD::StaticClass();
}

const ULuxPawnData* ALuxGameMode::GetPawnDataForController(const AController* InController) const
{
	ULuxAssetManager& AssetManager = ULuxAssetManager::Get();
	if (!InController)
	{
		return AssetManager.GetDefaultPawnData();
	}

	const ALuxPlayerState* PS = InController->GetPlayerState<ALuxPlayerState>();
	if (!PS)
	{
		return ULuxAssetManager::Get().GetDefaultPawnData();
	}

	// �̹� PlayerState�� ĳ�̵� PawnData
	if (const ULuxPawnData* Existing = PS->GetPawnData<ULuxPawnData>())
	{
		return Existing;
	}

	// ���õ� ID
	FPrimaryAssetId Id = PS->GetSelectedCharacterId();
	if (!Id.IsValid())
	{
		return ULuxAssetManager::Get().GetDefaultPawnData();
	}

	// �ε�� ��쿡�� �� ������ ��ȯ
	ULuxPawnDataManagerComponent* Mgr = GameState ? GameState->FindComponentByClass<ULuxPawnDataManagerComponent>() : nullptr;
	if (Mgr && Mgr->IsPawnDataLoaded(Id))
	{
		return Mgr->GetPawnDataChecked(Id);
	}

	// ���� �ε� ���̰ų� ��û���� �ʾҴٸ� �⺻��
	return ULuxAssetManager::Get().GetDefaultPawnData();
}


void ALuxGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

}

void ALuxGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALuxGameState* GS = GetGameState<ALuxGameState>())
	{
		if (ULuxPawnDataManagerComponent* Manager = GS->GetPawnDataManagerComponent())
		{
			Manager->UnregisterDelegates(this);
		}
	}

	// PawnDataRequestMap 정리
	PawnDataRequestMap.Empty();
	UE_LOG(LogLux, Log, TEXT("EndPlay: PawnDataRequestMap 정리 완료"));

	Super::EndPlay(EndPlayReason);
}

void ALuxGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 1. 재접속 플레이어인지 확인
	// 2. URL 옵션에서 PawnData ID를 확인
	// 3. AssetManager에서 기본 PawnData를 가져옴

	ALuxPlayerState* PS = NewPlayer->GetPlayerState<ALuxPlayerState>();
	if (ensure(PS) && PS->GetPawnData<ULuxPawnData>())
	{
		// 재접속 플레이어의 경우 이미 로드된 PawnData를 사용
		const ULuxPawnData* ExistingPawnData = PS->GetPawnData<ULuxPawnData>();
		UE_LOG(LogLux, Log, TEXT("PostLogin: 재접속 플레이어 '%s'가 기존 PawnData '%s'를 사용하여 캐릭터를 생성합니다."), 
			*NewPlayer->GetName(), *ExistingPawnData->GetPrimaryAssetId().ToString());
		PawnDataRequestMap.Add(ExistingPawnData->GetPrimaryAssetId(), NewPlayer);
		HandlePawnDataLoaded(ExistingPawnData);
		return;
	}

	FPrimaryAssetId PawnDataId;
	UNetConnection* Conn = NewPlayer->GetNetConnection();
	if (Conn)
	{
		FString PawnDataName = Conn->URL.GetOption(TEXT("PawnData"), TEXT(""));
		if (!PawnDataName.IsEmpty())
		{
			PawnDataId = FPrimaryAssetId(TEXT("LuxPawnData"), FName(*PawnDataName));
		}
	}

	if (!PawnDataId.IsValid())
	{
		ULuxAssetManager& AM = ULuxAssetManager::Get();

		const ULuxPawnData* DefaultPawnData = AM.GetDefaultPawnData();
		if (DefaultPawnData)
		{
			PawnDataId = DefaultPawnData->GetPrimaryAssetId();
		}
		else
		{
			// 기본값마저 없다면, 심각한 설정 오류이므로 접속을 종료하거나 에러를 기록합니다.
			UE_LOG(LogLux, Error, TEXT("FATAL: AssetManager 에 기본 PawnData가 설정되어 있지 않습니다. 플레이어를 접속 종료합니다."));
			NewPlayer->ClientTravel(TEXT("/Game/Maps/MainMenu"), ETravelType::TRAVEL_Absolute);
			return;
		}
	}

	ALuxGameState* GS = GetGameState<ALuxGameState>();
	check(GS);
	ULuxPawnDataManagerComponent* Manager = GS->GetPawnDataManagerComponent();
	check(Manager);

	// PawnData 요청을 TMap에 저장
	PawnDataRequestMap.Add(PawnDataId, NewPlayer);
	UE_LOG(LogLux, Log, TEXT("[%s] Controller '%s'가 PawnData '%s' 요청을 등록합니다."), ANSI_TO_TCHAR(__FUNCTION__), *NewPlayer->GetName(), *PawnDataId.ToString());

	if (Manager->IsPawnDataLoaded(PawnDataId))
	{
		const ULuxPawnData* LoadedData = Manager->GetPawnDataChecked(PawnDataId);
		HandlePawnDataLoaded(LoadedData);
	}
	else
	{
		Manager->CallOrRegister_OnPawnDataLoaded(PawnDataId, FOnPawnDataLoaded::FDelegate::CreateUObject(this, &ThisClass::HandlePawnDataLoaded));
		Manager->StartPawnDataLoad(PawnDataId);
	}
}

void ALuxGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		// 해당 Controller가 요청한 모든 PawnData를 TMap에서 제거
		TArray<FPrimaryAssetId> KeysToRemove;
		for (const auto& Pair : PawnDataRequestMap)
		{
			if (Pair.Value == Exiting)
			{
				KeysToRemove.Add(Pair.Key);
			}
		}
		
		for (const FPrimaryAssetId& Key : KeysToRemove)
		{
			PawnDataRequestMap.Remove(Key);
			UE_LOG(LogLux, Log, TEXT("Logout: Controller '%s'의 PawnData '%s' 요청을 TMap에서 제거"), *Exiting->GetName(), *Key.ToString());
		}
	}

	Super::Logout(Exiting);
}


void ALuxGameMode::HandlePawnDataLoaded(const ULuxPawnData* LoadedData)
{
	if (!LoadedData) return;

	// PawnDataRequestMap에서 해당 PawnData를 요청한 Controller를 찾습니다.
	FPrimaryAssetId PawnDataId = LoadedData->GetPrimaryAssetId();
	AController* Controller = *PawnDataRequestMap.Find(PawnDataId);
	PawnDataRequestMap.Remove(PawnDataId);

	if (!Controller) 
	{
		UE_LOG(LogLux, Warning, TEXT("[%s] LuxPawnData '%s' 로드 완료, 요청한 Controller 가 없습니다."), 
			ANSI_TO_TCHAR(__FUNCTION__), *PawnDataId.ToString());
		return;
	}

	UClass* PawnClass = LoadedData->PawnClass;
	if (!PawnClass)
	{
		UE_LOG(LogLux, Error, TEXT("[%s] '%s' 의 PawnClass 가 설정되어 있지 않습니다."), ANSI_TO_TCHAR(__FUNCTION__), *PawnDataId.ToString());
		return;
	}

	ALuxPlayerState* PS = Controller->GetPlayerState<ALuxPlayerState>();
	if (PS)
	{
		PS->SetPawnData(LoadedData);
	}

	FTransform SpawnTransform = ChoosePlayerStart(Controller)->GetTransform();
	APawn* Pawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform);
	if (Pawn)
	{
		Controller->Possess(Pawn);
	}

	ULuxPawnExtensionComponent* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (PawnExtComp)
	{
		PawnExtComp->SetPawnData(LoadedData);
	}
	else
	{
		UE_LOG(LogLux, Error, TEXT("[%s] '%s' 의 PawnExtensionComponent 가 설정되어 있지 않습니다."), ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(Pawn));
	}
}

UClass* ALuxGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const ULuxPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ALuxGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.bDeferConstruction = true;

	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (!PawnClass)
	{
		UE_LOG(LogLux, Error, TEXT("Game mode was unable to spawn Pawn due to NULL pawn class."));
		return nullptr;
	}

	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnParams);
	if (!SpawnedPawn)
	{
		UE_LOG(LogLux, Error, TEXT("Game mode was unable to spawn Pawn of class [%s] at [%s]."), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
		return nullptr;
	}

	ULuxPawnExtensionComponent* PawnExtComp = ULuxPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn);
	if (!PawnExtComp)
	{
		SpawnedPawn->FinishSpawning(SpawnTransform);
		return SpawnedPawn;
	}

	const ULuxPawnData* PawnData = GetPawnDataForController(NewPlayer);
	if (PawnData)
	{
		PawnExtComp->SetPawnData(PawnData);
	}
	else
	{
		UE_LOG(LogLux, Error, TEXT("Game mode was unable to set PawnData on the spawned pawn [%s]."), *GetNameSafe(SpawnedPawn));
	}

	SpawnedPawn->FinishSpawning(SpawnTransform);
	return SpawnedPawn;
}

bool ALuxGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return true;
}

void ALuxGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 의도적으로 비워서 표준 흐름을 사용하지 않도록 하였습니다.
}

AActor* ALuxGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	return Super::ChoosePlayerStart_Implementation(Player);
}

void ALuxGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool ALuxGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return true;
}

void ALuxGameMode::InitGameState()
{
	Super::InitGameState();
}

bool ALuxGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	return true;
}

void ALuxGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
}


void ALuxGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		TWeakObjectPtr<APlayerController> WeakPC = PC;
		GetWorldTimerManager().SetTimerForNextTick([WeakPC](){
				if (WeakPC.IsValid()) WeakPC->ServerRestartPlayer_Implementation();
		});
	}
	else if (APlayerController* BotController = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(BotController, &APlayerController::ServerRestartPlayer_Implementation);
	}
}

void ALuxGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (!PawnClass)
	{
		UE_LOG(LogLux, Warning, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."), *GetPathNameSafe(NewPlayer));
		return;
	}

	APlayerController* NewPC = Cast<APlayerController>(NewPlayer);
	if (!NewPC)
	{
		RequestPlayerRestartNextFrame(NewPlayer, false);
		return;
	}

	if (PlayerCanRestart(NewPC))
	{
		RequestPlayerRestartNextFrame(NewPlayer, false);
	}
	else
	{
		UE_LOG(LogLux, Warning, TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."), *GetPathNameSafe(NewPlayer));
	}
}

bool ALuxGameMode::IsPawnDataLoadedForController(const AController* InController) const
{
	const ALuxPlayerState* PS = InController->GetPlayerState<ALuxPlayerState>();
	if (!PS)
	{
		return false;
	}

	FPrimaryAssetId Id = PS->GetSelectedCharacterId();
	if (!Id.IsValid())
	{
		return false;
	}

	ULuxPawnDataManagerComponent* Mgr = GameState ? GameState->FindComponentByClass<ULuxPawnDataManagerComponent>() : nullptr;
	return Mgr && Mgr->IsPawnDataLoaded(Id);
}