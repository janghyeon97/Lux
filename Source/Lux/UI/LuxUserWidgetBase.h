// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LuxUserWidgetBase.generated.h"


class UActionSystemComponent;


UCLASS(Abstract)
class LUX_API ULuxUserWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Lux UI")
    virtual void InitializeWidget(UActionSystemComponent* InASC);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Lux UI")
    TWeakObjectPtr<UActionSystemComponent> ASC;
};