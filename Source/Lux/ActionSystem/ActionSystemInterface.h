// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"
#include "ActionSystemInterface.generated.h"

class UActionSystemComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UActionSystemInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class LUX_API IActionSystemInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual UActionSystemComponent* GetActionSystemComponent() const = 0;
};