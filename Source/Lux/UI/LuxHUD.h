// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LuxHUD.generated.h"

class UActionSystemComponent;

/**
 * 
 */
UCLASS()
class LUX_API ALuxHUD : public AHUD
{
	GENERATED_BODY()
	
public:
    ALuxHUD();

    virtual void DrawHUD() override;

private:
    /** ActionSystemComponent의 디버그 정보를 그리는 함수입니다. */
    void DrawActionSystemDebugInfo();

	/** LuxCameraComponent의 디버그 정보를 그리는 함수입니다. */
	void DrawCameraDebugInfo();

	/** 디버그 정보의 각 섹션을 그리는 헬퍼 함수들입니다. */
	void DrawDebugHeader(const UActionSystemComponent* ASC, float& PosY) const;
	void DrawAttributesInfo(const UActionSystemComponent* ASC, float& PosY) const;
	void DrawGrantedActionsInfo(const UActionSystemComponent* ASC, float& PosY) const;
	void DrawActiveActionsInfo(const UActionSystemComponent* ASC, float& PosY) const;
	void DrawActiveEffectsInfo(const UActionSystemComponent* ASC, float& PosY) const;
	void DrawGrantedTagsInfo(const UActionSystemComponent* ASC, float& PosY) const;
};
