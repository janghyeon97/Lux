// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayCueInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagAssetInterface.h"
#include "Teams/LuxTeamAgentInterface.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "LuxCharacter.generated.h"

class UObject;
class AActor;
class APlayerState;
class AController;

class UInputComponent;
class UHealthComponent;
class UGameplayEffect;
class ULuxPawnExtensionComponent;

struct FGameplayTag;
struct FGameplayTagContainer;

UCLASS()
class LUX_API ALuxCharacter : public ACharacter, public ILuxTeamAgentInterface, public IActionSystemInterface
{
	GENERATED_BODY()

public:
	ALuxCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** LuxActionSystem 을 반환합니다. */
	virtual UActionSystemComponent* GetActionSystemComponent() const override;

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	//~End of APawn interface

	//~ILuxTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILuxTeamAgentInterface interface

	/** 액션(스킬)에 의해 캐릭터의 움직임이 강제되고 있는지 여부를 설정합니다. */
	void SetActionControlledMovement(bool bIsActionControlled);

protected:
	//~ACharacter interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	//~End of ACharacter interface

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// 캐릭터의 죽음 이벤트를 시작합니다 (충돌 비활성화, 이동 비활성화 등)
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	// 캐릭터의 죽음 이벤트를 종료합니다 (컨트롤러 분리, 폰 파괴 등)
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	// Blueprint 이벤트
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnDeathFinished"))
	void K2_OnDeathFinished();

	/** MoveSpeed 속성이 변경되었을 때 호출될 콜백 함수입니다. */
	UFUNCTION()
	void OnMoveSpeedChanged(float OldValue, float NewValue);

	void DisableMovementAndCollision();
	void DestroyDueToDeath();
	void UninitAndDestroy();

	/* 액션 시스템이 초기화 완료 후 호출됩니다. */
	UFUNCTION()
	virtual void OnActionSystemInitialized();

	/* 액션 시스템이 해제될 때 호출됩니다. */
	UFUNCTION()
	virtual void OnActionSystemUninitialized();

public:
	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

protected:
	// UnPossessed 시 팀 ID 를 결정합니다.
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// return FGenericTeamId(ETeamAttitude::Neutral);
		return OldTeamID;
	}

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULuxPawnExtensionComponent> PawnExtComponent;

	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> HealthComponent;*/

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

public:
	/** 액션(스킬)에 의해 캐릭터의 움직임이 강제되고 있는지 여부입니다. */
	UPROPERTY(Replicated)
	bool bIsActionControlledMovement;

	static FString ClientServerStatus;
	static FName NAME_LuxCharacterCollisionProfile_Capsule;
	static FName NAME_LuxCharacterCollisionProfile_Mesh;
};
