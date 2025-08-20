// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "ActorInitData.generated.h"

class UActionSystemComponent;


USTRUCT(BlueprintType)
struct FActorInitDataBase
{
    GENERATED_BODY()

public:
    virtual ~FActorInitDataBase() {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UActionSystemComponent> SourceASC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize OriginLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator OriginRotation = FRotator::ZeroRotator;
};