// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/LuxInputComponent.h"
#include "EnhancedInputSubsystems.h"

class ULuxInputConfig;

ULuxInputComponent::ULuxInputComponent(const FObjectInitializer& ObjectInitializer)
{

}

void ULuxInputComponent::AddInputMappings(const ULuxInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

}

void ULuxInputComponent::RemoveInputMappings(const ULuxInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

}
