// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LuxUserWidgetBase.h"
#include "ActionSystem/ActionSystemComponent.h"

void ULuxUserWidgetBase::InitializeWidget(UActionSystemComponent* InASC)
{
    if (InASC)
    {
        ASC = InASC;
    }
}