// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/PawnComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"

#include "LuxPawnExtensionComponent.generated.h"


class UGameFrameworkComponentManager;
class UActionSystemComponent;
class ULuxPawnData;
class UObject;


/**
 * 
 */
UCLASS()
class LUX_API ULuxPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()
	

public:
	ULuxPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The name of this overall feature, this one depends on the other named component features */
	static const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	/** LuxPawnExtensionComponent �� ã�� ��ȯ�մϴ�. ���� ��� nullptr */
	UFUNCTION(BlueprintPure, Category = "Lux|Pawn")
	static ULuxPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<ULuxPawnExtensionComponent>() : nullptr);
	}

	/** PawnData 를 반환합니다. */
	template <class T>
	const T* GetPawnData() const
	{
		return Cast<T>(PawnData);
	}

	/** PawnData 를 설정합니다. */
	void SetPawnData(const ULuxPawnData* InPawnData);

	/** 액션 시스템 컴포넌트를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Lux|Pawn")
	UActionSystemComponent* GetActionSystemComponent() const
	{
		return ActionSystemComponent;
	}

	/** 액션 시스템 초기화를 시도하는 함수입니다. */
	void InitializeActionSystem();

	/** Pawn의 액션 시스템을 아바타에서 해제할 때 호출해야 합니다. */
	void UninitializeActionSystem();

	/** 컨트롤러가 변경될 때 호출해야 합니다. */
	void HandleControllerChanged();

	/** PlayerState가 Replicate 될 때 호출해야 합니다. */
	void HandlePlayerStateReplicated();

	/** PlayerInputComponent를 설정할 때 호출해야 합니다. */
	void SetupPlayerInputComponent();

	/**
	 * 액션 시스템 초기화 완료 시 실행되는 델리게이트를 등록합니다.
	 * Pawn이 이미 빙의되어 있는 경우 즉시 호출합니다.
	 */
	void OnActionSystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);

	/**
	 * Pawn의 액션 시스템이 아바타에서 해제될 때 발생하는 델리게이트를 등록합니다.
	 */
	void OnActionSystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate);

protected:
	// Pawn의 액션 시스템이 아바타 인터페이스로 초기화될 때 실행되는 델리게이트
	FSimpleMulticastDelegate OnActionSystemInitialized;

	// Pawn의 액션 시스템이 아바타 인터페이스에서 해제될 때 실행되는 델리게이트
	FSimpleMulticastDelegate OnActionSystemUninitialized;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_PawnData();

	UPROPERTY(EditInstanceOnly, ReplicatedUsing = OnRep_PawnData, Category = "Lux|Pawn")
	TObjectPtr<const ULuxPawnData> PawnData;

	UPROPERTY(Transient)
	TObjectPtr<UActionSystemComponent> ActionSystemComponent;
};