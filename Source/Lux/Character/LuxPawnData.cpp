// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LuxPawnData.h"

ULuxPawnData::ULuxPawnData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PawnClass = nullptr;
	InputConfig = nullptr;
}
