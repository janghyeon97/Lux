// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "ActionSystem/ActionSystemComponent.h"
#include "Net/UnrealNetwork.h"


/** 일시적인 버프가 모두 포함된 현재 값을 반환합니다. */
float FLuxAttributeData::GetCurrentValue() const
{
    return CurrentValue;
}

/** 현재 값을 변경합니다. */
void FLuxAttributeData::SetCurrentValue(float NewValue)
{
    CurrentValue = NewValue;
}

/** 영구적인 변화가 반영된 Base 값을 반환합니다. */
float FLuxAttributeData::GetBaseValue() const 
{
    return BaseValue;
}

/** 영구적으로 Base 값을 변경합니다. */
void FLuxAttributeData::SetBaseValue(float NewValue)
{
    BaseValue = NewValue;
}

//--------------------------------------------------------------------------------------------------------------

ULuxAttributeSet::ULuxAttributeSet()
{

}

bool FLuxAttribute::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    Ar << Attribute;

    bOutSuccess = true;
    return true;
}

void ULuxAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

AActor* ULuxAttributeSet::GetOwningActor() const
{
    return CastChecked<AActor>(GetOuter());
}

UActionSystemComponent* ULuxAttributeSet::GetOwningActionSystemComponent() const
{
    AActor* OwningActor = GetOwningActor();
    return OwningActor->FindComponentByClass<UActionSystemComponent>();
}

UActionSystemComponent* ULuxAttributeSet::GetOwningActionSystemComponentChecked() const
{
    UActionSystemComponent* ASC = GetOwningActionSystemComponent();
    check(ASC);
    return ASC;
}

bool FLuxAttribute::IsValid() const
{
    return Attribute.Get() != nullptr;
}

FProperty* FLuxAttribute::GetProperty() const
{
    return Attribute.Get();
}

UClass* FLuxAttribute::GetAttributeSetClass() const
{
    if (!FLuxAttribute::IsValid()) 
        return nullptr;
    return Attribute->GetOwner<UClass>();
}

const FLuxAttributeData* FLuxAttribute::GetAttributeData(const ULuxAttributeSet* Src) const
{
    if (!FLuxAttribute::IsValid() || !Src) 
        return nullptr;

    FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.Get());
    if (!StructProperty)
    {
        return nullptr;
    }

    if (StructProperty->Struct != FLuxAttributeData::StaticStruct())
    {
        return nullptr;
    }

    return StructProperty->ContainerPtrToValuePtr<FLuxAttributeData>(Src);
}

const FLuxAttributeData* FLuxAttribute::GetAttributeDataChecked(const ULuxAttributeSet* Src) const
{
    check(IsValid() && Src);
    FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.Get());
    check(StructProperty && StructProperty->Struct == FLuxAttributeData::StaticStruct());
    return StructProperty->ContainerPtrToValuePtr<FLuxAttributeData>(Src);
}

FLuxAttributeData* FLuxAttribute::GetAttributeData(ULuxAttributeSet* Src) const
{
    if (!FLuxAttribute::IsValid() || !Src)
        return nullptr;

    FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.Get());
    if (!StructProperty)
    {
        return nullptr;
    }

    if (StructProperty->Struct != FLuxAttributeData::StaticStruct())
    {
        return nullptr;
    }

    return StructProperty->ContainerPtrToValuePtr<FLuxAttributeData>(Src);
}

FLuxAttributeData* FLuxAttribute::GetAttributeDataChecked(ULuxAttributeSet* Src) const
{
    check(IsValid() && Src);
    FStructProperty* StructProperty = CastField<FStructProperty>(Attribute.Get());
    check(StructProperty && StructProperty->Struct == FLuxAttributeData::StaticStruct());
    return StructProperty->ContainerPtrToValuePtr<FLuxAttributeData>(Src);
}

bool FLuxAttribute::operator==(const FLuxAttribute& Other) const
{
    return Attribute == Other.Attribute;
}