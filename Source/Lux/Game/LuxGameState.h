// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameState.h"
#include "LuxGameState.generated.h"

class APlayerState;
class UActionSystemComponent;
class UActionSystemComponent;
class ULuxPawnDataManagerComponent;
class UObject;


/**
 * 
 */
UCLASS()
class LUX_API ALuxGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ALuxGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	//~IActionSystemInterface
	UActionSystemComponent* GetActionSystemComponent() const;
	//~End of IActionSystemInterface

	UFUNCTION(BlueprintCallable, Category = "Lux|GameState")
	UActionSystemComponent* GetLuxActionSystemComponent() const { return ActionSystemComponent; }

	ULuxPawnDataManagerComponent* GetPawnDataManagerComponent() const { return PawnDataManager; };

private:
	UPROPERTY(VisibleAnywhere, Category = "Lux|GameState")
	TObjectPtr<UActionSystemComponent> ActionSystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "Lux|GameState")
	TObjectPtr<ULuxPawnDataManagerComponent> PawnDataManager;
};
