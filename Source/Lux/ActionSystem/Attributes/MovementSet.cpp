// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Attributes/MovementSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"


UMovementSet::UMovementSet()
{
    InitMoveSpeed(325.f);
}

void UMovementSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMovementSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UMovementSet::PreAttributeChange(const FLuxAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // 이동 속도가 비정상적인 값(예: 음수)이 되지 않도록 최소값을 제한합니다.
    if (Attribute == GetMoveSpeedAttribute())
    {
        NewValue = FMath::Max(0.f, NewValue);
    }
}

void UMovementSet::PostAttributeChange(const FLuxAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMoveSpeedAttribute())
    {
        OnMoveSpeedChanged.Broadcast(OldValue, NewValue);
    }
}

void UMovementSet::OnRep_MoveSpeed(const FLuxAttributeData& OldValue)
{
    // 클라이언트에서도 복제된 값이 변경되었을 때 이동 속도를 갱신하기 위해 PostAttributeChange를 호출합니다.
    LUXATTRIBUTE_REPNOTIFY(UMovementSet, MoveSpeed, OldValue);
    PostAttributeChange(GetMoveSpeedAttribute(), OldValue.GetCurrentValue(), MoveSpeed.GetCurrentValue());
}
