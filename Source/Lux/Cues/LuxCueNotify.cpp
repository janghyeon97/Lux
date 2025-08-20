// Fill out your copyright notice in the Description page of Project Settings.


#include "Cues/LuxCueNotify.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "GameFramework/Character.h"

FLuxCueContext::FLuxCueContext()
    : Instigator(nullptr)
    , EffectCauser(nullptr)
    , SourceASC(nullptr)
    , Location(FVector::ZeroVector)
    , Normal(FVector::ZeroVector)
    , Rotation(FRotator::ZeroRotator)
    , Magnitude(0.0f)
    , Duration(0.0f)
    , Rate(0.0f)
    , Count(0)
    , Lifetime(0.0f)
    , Level(1)
{
}

FLuxCueContext::FLuxCueContext(const FLuxCueContext& Other)
    : Instigator(Other.Instigator)
    , EffectCauser(Other.EffectCauser)
    , SourceASC(Other.SourceASC)
    , Location(Other.Location)
    , Normal(Other.Normal)
    , Rotation(Other.Rotation)
    , Magnitude(Other.Magnitude)
    , Duration(Other.Duration)
    , Rate(Other.Rate)
    , Count(Other.Count)
    , Lifetime(Other.Lifetime)
    , Level(Other.Level)
    , CueTags(Other.CueTags)
{
}

FLuxCueContext& FLuxCueContext::operator=(const FLuxCueContext& Other)
{
    if (this == &Other)
    {
        return *this;
    }

    Instigator = Other.Instigator;
    EffectCauser = Other.EffectCauser;
    SourceASC = Other.SourceASC;
    Location = Other.Location;
    Normal = Other.Normal;
    Rotation = Other.Rotation;
    Magnitude = Other.Magnitude;
    Duration = Other.Duration;
    Rate = Other.Rate;
    Count = Other.Count;
    Lifetime = Other.Lifetime;
    Level = Other.Level;
    CueTags = Other.CueTags;

    return *this;
}

ALuxCueNotify::ALuxCueNotify()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetCanBeDamaged(false);
}


void ALuxCueNotify::Execute(AActor* Target, const FLuxCueContext& Context)
{
    CueTarget = Target;
    InitialContext = Context;

    if (bAutoAttachToTarget && Target)
    {
        ACharacter* TargetCharacter = Cast<ACharacter>(Target);
        if (TargetCharacter)
        {
            AttachToComponent(TargetCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
        }
        else
        {
            AttachToActor(Target, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
        }
    }

    OnActive(Target, Context);
    OnCueActivated.ExecuteIfBound(this);
    SetActorHiddenInGame(false);
    SetActorTickEnabled(true);
}

void ALuxCueNotify::Stop()
{
    OnStopped(CueTarget.Get(), InitialContext);
    OnCueStopped.ExecuteIfBound(this);
    SetActorTickEnabled(false);
}

void ALuxCueNotify::OnActive_Implementation(AActor* Target, const FLuxCueContext& Context)
{

}

void ALuxCueNotify::OnStopped_Implementation(AActor* Target, const FLuxCueContext& Context)
{
    
}
