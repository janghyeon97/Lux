// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/LuxActionSystemTypes.h"


bool FOwningActorInfo::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

	uint8 RepBits = 0;
	if (Ar.IsSaving())
	{
		if (OwnerActor.IsValid()) RepBits |= 1 << 0;
		if (AvatarActor.IsValid()) RepBits |= 1 << 1;
		if (ActionSystemComponent.IsValid()) RepBits |= 1 << 2;
		if (Controller.IsValid()) RepBits |= 1 << 3;
	}

	Ar.SerializeBits(&RepBits, 4);

	if (RepBits & (1 << 0)) Ar << OwnerActor;
	if (RepBits & (1 << 1)) Ar << AvatarActor;
	if (RepBits & (1 << 2)) Ar << ActionSystemComponent;
	if (RepBits & (1 << 3)) Ar << Controller;

	return bOutSuccess;
}


bool FOwningActorInfo::operator==(const FOwningActorInfo& Other) const
{
	if (OwnerActor != Other.OwnerActor)
	{
		return false;
	}

	if (AvatarActor != Other.AvatarActor)
	{
		return false;
	}

	if (ActionSystemComponent != Other.ActionSystemComponent)
	{
		return false;
	}

	if (Controller != Other.Controller)
	{
		return false;
	}

	return true;
}

bool FOwningActorInfo::operator!=(const FOwningActorInfo& Other) const
{
	if (OwnerActor != Other.OwnerActor)
	{
		return true;
	}

	if (AvatarActor != Other.AvatarActor)
	{
		return true;
	}

	if (ActionSystemComponent != Other.ActionSystemComponent)
	{
		return true;
	}

	if (Controller != Other.Controller)
	{
		return true;
	}

	return false;
}
