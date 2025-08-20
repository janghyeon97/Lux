// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerState.h"
#include "Teams/LuxTeamAgentInterface.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "LuxPlayerState.generated.h"


class AController;
class ALuxPlayerController;
class APlayerState;
class FName;
class UActionSystemComponent;
class ULuxPawnData;
class UHealthSet;
struct FGameplayTag;


/** 클라이언트 연결 유형을 정의합니다 */
UENUM()
enum class ELuxPlayerConnectionType : uint8
{
	/** 활성 플레이어 */
	Player = 0,

	/** 실시간 게임에 접속한 관전자 */
	LiveSpectator,

	/** 리플레이 녹화본을 시청하는 관전자 */
	ReplaySpectator,

	/** 비활성화된 플레이어 (연결 끊김 등) */
	InactivePlayer
};


/**
 * 
 */
UCLASS()
class LUX_API ALuxPlayerState : public APlayerState, public ILuxTeamAgentInterface, public IActionSystemInterface
{
	GENERATED_BODY()
	

public:
	ALuxPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Lux|PlayerState")
	ALuxPlayerController* GetLuxPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "Lux|PlayerState")
	virtual UActionSystemComponent* GetActionSystemComponent() const override;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }
	void SetPawnData(const ULuxPawnData* InPawnData);

	void SetPlayerConnectionType(ELuxPlayerConnectionType NewType);
	ELuxPlayerConnectionType GetPlayerConnectionType() const { return PlayerConnectionType; }

	void SetCharacterId(FPrimaryAssetId CharacterId) { SelectedCharacterId = CharacterId; };
	FPrimaryAssetId GetSelectedCharacterId() const { return SelectedCharacterId; };

	/** �÷��̾ ���� ���� �� ID�� ��ȯ�մϴ�. */
	UFUNCTION(BlueprintCallable)
	int32 GetTeamId() const
	{
		return GenericTeamIdToInteger(MyTeamID);
	}

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~End of APlayerState interface

	//~ILuxTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILuxTeamAgentInterface interface

public:
	static const FName NAME_LuxAbilityReady;

protected:
	UFUNCTION()
	void OnRep_PawnData();

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const ULuxPawnData> PawnData;

private:
	UPROPERTY()
	UActionSystemComponent* ActionSystemComponent;

	UPROPERTY(Replicated)
	ELuxPlayerConnectionType PlayerConnectionType;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY()
	FPrimaryAssetId SelectedCharacterId;

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);
};
