// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LuxPlayerController.generated.h"


class UUserWidget;
class UCameraShakeBase;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetAcquired, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetingCancelled);


/**
 *
 */
UCLASS()
class LUX_API ALuxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALuxPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	//~ AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void AcknowledgePossession(APawn* PossessedPawn) override;
	//~ End of AController interface

#pragma region UI / HUD
	/* ======================================== UI / HUD ======================================== */
public:
	/** 화면에 데미지 수치를 표시하도록 HUD에 요청합니다. */
	UFUNCTION(Client, Unreliable)
	void Client_ShowDamageNumber(float DamageAmount, AActor* TargetActor);

	/** 화면에 알림 메시지를 표시하도록 HUD에 요청합니다. */
	UFUNCTION(Client, Unreliable)
	void Client_ShowNotification(const FText& Message);

	/** 액션 시스템 디버그 정보 표시를 토글합니다. */
	void ToggleActionSystemDebug();

	/** 메인 HUD 위젯을 표시합니다. */
	UFUNCTION(BlueprintCallable)
	void ShowMainHUDWidget();

	/** 메인 HUD 위젯을 숨깁니다. */
	UFUNCTION(BlueprintCallable)
	void HideMainHUDWidget();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainHUDWidgetClass;

	/** 액션 시스템 디버그 정보 표시 여부 */
	bool bShowActionSystemDebug = false;
protected:
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> MainHUDWidget;
#pragma endregion


#pragma region Targeting
	/* ======================================== Targeting ======================================== */
public:
	/** 타겟팅 모드를 시작합니다. */
	UFUNCTION(BlueprintCallable)
	void BeginTargeting();

	/** 타겟팅 모드를 종료합니다. */
	UFUNCTION(BlueprintCallable)
	void EndTargeting();

	/** 타겟이 성공적으로 지정되었을 때 브로드캐스트됩니다. */
	UPROPERTY(BlueprintAssignable)
	FOnTargetAcquired OnTargetAcquired;

	/** 타겟 지정이 취소되었을 때 브로드캐스트됩니다. */
	UPROPERTY(BlueprintAssignable)
	FOnTargetingCancelled OnTargetingCancelled;

protected:
	/** 타겟팅 모드에서 확인 클릭이 눌렸을 때 호출될 함수입니다. */
	void OnTargetingConfirm();

	/** 타겟팅 모드에서 마우스 오른쪽 클릭이 발생했을 때 호출될 함수입니다. */
	void OnTargetingCancel();

	/** 현재 타겟팅 모드인지 여부 */
	bool bIsTargeting;
#pragma endregion


#pragma region Member Variables
	/* ======================================== Member Variables ======================================== */

#pragma endregion

	/* 액션 시스템이 초기화 완료 후 호출됩니다. */
	UFUNCTION()
	virtual void OnActionSystemInitialized();

	/* 액션 시스템이 해제될 때 호출됩니다. */
	UFUNCTION()
	virtual void OnActionSystemUninitialized();

	FDelegateHandle OnPawnInitStateChangedHandle;
};
