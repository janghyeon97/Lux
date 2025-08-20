// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/VitalsWidget.h"
#include "UI/AttributeBarBase.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "LuxLogChannels.h"

void UVitalsWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UVitalsWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UVitalsWidget::InitializeWidget(UActionSystemComponent* InASC)
{
	Super::InitializeWidget(InASC);

    if (HealthBar)
    {
        HealthBar->InitializeWidget(InASC);
    }

    if (ManaBar)
    {
        ManaBar->InitializeWidget(InASC);
    }

    if (ExperienceBar)
    {
        ExperienceBar->InitializeWidget(InASC);
    }
}





