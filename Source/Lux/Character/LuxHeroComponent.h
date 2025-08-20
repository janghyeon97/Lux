// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "LuxHeroComponent.generated.h"

namespace EEndPlayReason { enum Type : int; }
struct FLoadedMappableConfigPair;
struct FMappableConfigPair;

class UEnhancedInputLocalPlayerSubsystem;
class UGameFrameworkComponentManager;
class UInputComponent;
class UInputMappingContext;
class ULuxCameraMode;
class UInputConfig;
class UObject;
struct FActorInitStateChangedParams;
struct FFrame;
struct FGameplayTag;
struct FInputActionValue;
struct FInputMappingContext;

USTRUCT()
struct FInputMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input", meta = (AssetBundles = "Client,Server"))
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere, Category = "Input")
	int32 Priority = 0;
};

/**
 * Component that sets up input and camera handling for player controlled pawns (or bots that simulate players).
 * This depends on a PawnExtensionComponent to coordinate initialization.
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class LUX_API ULuxHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	ULuxHeroComponent(const FObjectInitializer& ObjectInitializer);

	/** 주어진 액터에 있는 히어로 컴포넌트를 찾아 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Lux|Hero")
	static ULuxHeroComponent* FindHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<ULuxHeroComponent>() : nullptr); }

	/** 활성화된 게임플레이 어빌리티(Gameplay Ability)를 위해 특정 카메라 모드를 설정합니다. */
	//void SetAbilityCameraMode(TSubclassOf<ULuxCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle);

	/** 어빌리티의 카메라 모드(override)를 해제합니다. */
	//void ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle);

	/** 런타임용 입력 설정(Input Config)을 추가합니다. */
	//void AddAdditionalInputConfig(const UInputConfig* InputConfig);

	/** 추가된 런타임용 입력 설정(Input Config)을 제거합니다. */
	//void RemoveAdditionalInputConfig(const UInputConfig* InputConfig);

	/** 로컬 플레이어가 존재하며, 추가 입력 매핑을 적용할 수 있을 만큼 초기화가 진행되었는지 여부를 반환합니다. */
	bool IsReadyToBindInputs() const;

	/** 어빌리티 입력 바인딩 준비가 완료되었을 때 UGameFrameworkComponentManager에 등록되는 확인 이벤트 이름입니다. */
	static const FName NAME_BindInputsNow;

	/** 이 컴포넌트를 지칭하는 기능(Feature) 이름입니다. */
	static const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	
	/** PawnData에서 기본 카메라 모드를 설정합니다. */
	void SetupDefaultCameraMode();
	
	void Input_ActionInputTagPressed(FGameplayTag InputTag);
	void Input_ActionInputTagReleased(FGameplayTag InputTag);

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	void Input_LookStick(const FInputActionValue& InputActionValue);
	void Input_Jump();

	void Input_ToggleMouseCursor();

	void SetupInputMappings(const APawn* Pawn, UEnhancedInputLocalPlayerSubsystem* Subsystem, UInputComponent* PlayerInputComponent);
	
public:
	/** 전/후 입력 값입니다. (-1.0 ~ 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Character Input")
	float ForwardInputValue = 0.0f;

	/** 좌/우 입력 값입니다. (-1.0 ~ 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Character Input")
	float RightInputValue = 0.0f;

	/** 현재 Controller 의 Yaw 값입니다. (-1.0 ~ 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Character Input")
	float AimYaw = 0.0f;

	/** 현재 Controller 의 Pitch 값입니다. (-1.0 ~ 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Character Input")
	float AimPitch = 0.0f;

protected:
	UPROPERTY(EditAnywhere)
	TArray<FInputMappingContextAndPriority> DefaultInputMappings;

	bool bReadyToBindInputs = false;

	/** Alt 키로 토글된 마우스 커서 표시 상태 */
	bool bIsMouseCursorVisible = false;
};
