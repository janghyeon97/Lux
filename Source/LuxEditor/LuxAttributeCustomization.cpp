// Fill out your copyright notice in the Description page of Project Settings.


#include "LuxAttributeCustomization.h"

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

#include "ActionSystem/Attributes/LuxAttributeSet.h"
#include "LuxLogChannels.h"

TSharedRef<IPropertyTypeCustomization> FLuxAttributeCustomization::MakeInstance()
{
    return MakeShareable(new FLuxAttributeCustomization());
}

void FLuxAttributeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    // FLuxAttribute 구조체 내부의 'Attribute' TFieldPath 타입 핸들을 가져옵니다.
    AttributeProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FLuxAttribute, Attribute));

    AttributePickerOptions.Empty();
    AttributePickerOptions.Add(MakeShareable(new FString("None")));

    for (TObjectIterator<UClass> It; It; ++It)
    {
        UClass* CurrentClass = *It;

        // 찾은 클래스가 ULuxAttributeSet의 자식이면서, 직접 생성 가능한 클래스인지 확인합니다.
        if (CurrentClass->IsChildOf(ULuxAttributeSet::StaticClass()) && !CurrentClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            // TFieldIterator를 사용하여 해당 클래스의 모든 프로퍼티(멤버 변수)를 순회합니다.
            for (TFieldIterator<FProperty> PropIt(CurrentClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
            {
                /*if ((*PropIt)->HasMetaData(TEXT("bHiddenInGame")))
                {
                    continue;
                }*/

                FProperty* Property = *PropIt;

                // 해당 프로퍼티가 FStructProperty이고, 그 타입이 FLuxAttributeData인지 확인합니다.
                if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
                {
                    if (StructProperty->Struct == FLuxAttributeData::StaticStruct())
                    {
                        // "Set이름.속성이름" 형식의 문자열을 만들어 옵션 목록에 추가합니다.
                        FString SetName = CurrentClass->GetName();
                        if (SetName.EndsWith(TEXT("_C")))
                        {
                            SetName.LeftChopInline(2);
                        }
                        FString AttrName = Property->GetName();
                        AttributePickerOptions.Add(MakeShareable(new FString(FString::Printf(TEXT("%s.%s"), *SetName, *AttrName))));
                    }
                }
            }
        }
    }

    // 자기 자신에 대한 약한 참조 포인터 생성합니다.
    TWeakPtr<FLuxAttributeCustomization> WeakThisPtr = StaticCastSharedRef<FLuxAttributeCustomization>(AsShared());

    SComboBox<TSharedPtr<FString>>::FOnSelectionChanged SelectionChangedHandler;
    SelectionChangedHandler.BindLambda([WeakThisPtr](TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
        {
            if (!WeakThisPtr.IsValid()) return;

            TSharedPtr<FLuxAttributeCustomization> PinnedThis = WeakThisPtr.Pin();
            if (!PinnedThis) return;

            PinnedThis->OnAttributeSelected(NewSelection, SelectInfo);
        });

    
    TAttribute<FText> TextBinding;
    TextBinding = TAttribute<FText>::Create([WeakThisPtr]() -> FText
        {
            if (!WeakThisPtr.IsValid()) 
                return FText::FromString("None");

            TSharedPtr<FLuxAttributeCustomization> PinnedThis = WeakThisPtr.Pin();
            if (!PinnedThis) return FText::FromString("None");

            return PinnedThis->GetCurrentAttributeName();
        });

    HeaderRow
        .NameContent()
        [
            PropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MinDesiredWidth(250)
        [
            SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&AttributePickerOptions)
                .OnSelectionChanged(SelectionChangedHandler)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
                    {
                        return SNew(STextBlock).Text(FText::FromString(*InItem));
                    })
                [
                    SNew(STextBlock)
                        .Text(TextBinding)
                ]
        ];
}

void FLuxAttributeCustomization::OnAttributeSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid() && AttributeProperty.IsValid())
    {
        FString FullString = *NewSelection.Get();
        FString ClassName;
        FString PropertyName;

        FullString.Split(TEXT("."), &ClassName, &PropertyName);

        UClass* FoundClass = UClass::TryFindTypeSlow<UClass>(ClassName);
        if (FoundClass)
        {
            FProperty* Property = FindFProperty<FProperty>(FoundClass, *PropertyName);
            if (Property)
            {
                AttributeProperty->SetValue(Property);
                return;
            }
        }

        FProperty* NullProperty = nullptr;
        AttributeProperty->SetValue(reinterpret_cast<UObject*&>(NullProperty));
    }
}

FText FLuxAttributeCustomization::GetCurrentAttributeName() const
{
    if (AttributeProperty.IsValid())
    {
        FString CurrentValue;
        AttributeProperty->GetValueAsFormattedString(CurrentValue);

        if (CurrentValue.IsEmpty())
        {
            return FText::FromString("None");
        }

        return FText::FromString(CurrentValue);
    }

    return FText::FromString("None");
}