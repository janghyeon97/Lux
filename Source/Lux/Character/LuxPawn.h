// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Teams/LuxTeamAgentInterface.h"
#include "LuxPawn.generated.h"


class ULuxPawnExtensionComponent;
class UActionSystemComponent;
class UPawnSensingComponent;
class UCapsuleComponent;


UCLASS()
class LUX_API ALuxPawn : public APawn, public ILuxTeamAgentInterface
{
	GENERATED_BODY()

public:
	ALuxPawn();

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~ILuxTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILuxTeamAgentInterface interface

protected:
	//~ACharacter interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	//~End of ACharacter interface

	virtual void OnRep_Controller() override;

	virtual void OnActionSystemInitialized();
	virtual void OnActionSystemUninitialized();

	// UnPossessed 시 팀 ID 를 결정합니다.
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// return FGenericTeamId(ETeamAttitude::Neutral);
		return OldTeamID;
	}

public:
	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Pawn", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULuxPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Pawn", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Pawn", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPawnSensingComponent> PawnSensingComponent;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;
};
