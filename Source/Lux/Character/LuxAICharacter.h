// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/LuxCharacter.h"
#include "Components/WidgetComponent.h"
#include "LuxAICharacter.generated.h"

class ULuxPawnData;
class UVitalsWidget;

/**
 * 
 */
UCLASS()
class LUX_API ALuxAICharacter : public ALuxCharacter
{
	GENERATED_BODY()
	
public:
	ALuxAICharacter();

	/** LuxActionSystem 을 반환합니다. */
	virtual UActionSystemComponent* GetActionSystemComponent() const override;

	/** 액션 시스템이 초기화 완료 후 호출됩니다. */
	virtual void OnActionSystemInitialized() override;

	/** VitalsWidget의 ExperienceBar를 숨깁니다. */
	void HideExperienceBar(UVitalsWidget* VitalsWidget);

protected:
	// 월드에 배치된 AI 캐릭터의 초기화를 위한 함수들
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	void OnDefaultPawnDataLoaded(const ULuxPawnData* PawnData);

	// PawnData 로딩 헬퍼 함수들
	bool TryLoadPawnDataById(const FPrimaryAssetId& PawnDataId);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UActionSystemComponent> ActionSystemComponent;

	/** 캐릭터 머리 위에 표시될 VitalsWidget을 위한 WidgetComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lux|AI|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> VitalsWidgetComponent;

	/** VitalsWidget 클래스 (에디터에서 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|AI|UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UVitalsWidget> VitalsWidgetClass;

	/** AI 캐릭터 전용 PawnData (에디터에서 설정 가능) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|AI|Data", meta = (AllowPrivateAccess = "true"))
	FPrimaryAssetId AIPawnDataId;

	/** AI 캐릭터 전용 PawnData 에셋 경로 (에디터에서 설정 가능) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lux|AI|Data", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<ULuxPawnData> AIPawnDataPath;
};
