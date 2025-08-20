// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LuxCharacterMovementComponent.h"
#include "ActionSystem/ActionSystemInterface.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "LuxGameplayTags.h"

float ULuxCharacterMovementComponent::GetMaxSpeed() const
{
	const IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(GetOwner());
	if (!ASCInterface)
	{
		return Super::GetMaxSpeed();
	}

	const UActionSystemComponent* ASC = ASCInterface->GetActionSystemComponent();

	// 만약 State_Block_Movement 태그가 있다면, 다른 모든 속도 계산을 무시하고 즉시 0을 반환합니다.
	if (ASC && ASC->HasTag(LuxGameplayTags::State_Block_Movement))
	{
		return 0.0f;
	}

	// 이동이 차단되지 않았다면 기존 로직(속성 값에 따른 MaxWalkSpeed 등)을 그대로 따릅니다.
	return Super::GetMaxSpeed();
}
