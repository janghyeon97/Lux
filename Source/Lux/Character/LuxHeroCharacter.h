// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/LuxCharacter.h"
#include "Targeting/LuxTargetingInterface.h"
#include "LuxHeroCharacter.generated.h"


class ULuxPawnData;
class ULuxHeroComponent;
class UCameraComponent;
class USpringArmComponent;
class ULuxCameraComponent;
class UTargetingComponent;

/**
 * 
 */
UCLASS()
class LUX_API ALuxHeroCharacter : public ALuxCharacter, public ILuxTargetingInterface
{
	GENERATED_BODY()
	
public:
	ALuxHeroCharacter(const FObjectInitializer& ObjectInitializer);

	/** LuxActionSystem 을 반환합니다. */
	virtual UActionSystemComponent* GetActionSystemComponent() const override;

	/* HeroComponent 를 반환합니다. */
	ULuxHeroComponent* GetHeroComponent() const { return HeroComponent; }

	/* 메인 카메라 컴포넌트를 반환합니다. */
	ULuxCameraComponent* GetCameraComponent() const { return CameraComponent; }

	/* 메인 스프링 암 컴포넌트를 반환합니다. */
	USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

	/* 타겟팅 컴포넌트를 반환합니다. */
	UTargetingComponent* GetTargetingComponent() const override { return TargetingComponent; }

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

protected:
	/** 현재 PawnData를 반환합니다. */
	const ULuxPawnData* GetPawnData() const;

protected:
	//~ACharacter interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	//~End of ACharacter interface

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	/* 액션 시스템이 초기화 완료 후 호출됩니다. */
	virtual void OnActionSystemInitialized() override;

	/* 액션 시스템이 해제될 때 호출됩니다. */
	virtual void OnActionSystemUninitialized() override;

protected:
	/* 플레이어 입력 및 영웅 관련 로직을 처리하는 핵심 컴포넌트입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Hero", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULuxHeroComponent> HeroComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Pawn", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Hero", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULuxCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Hero", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	/** 플레이어의 화면 중앙 타겟팅을 처리하는 컴포넌트입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|Hero", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTargetingComponent> TargetingComponent;
};
