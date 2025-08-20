// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class FLuxAttributeCustomization : public IPropertyTypeCustomization
{

public:
    /* 이 클래스의 인스턴스를 생성하는 static 함수 */
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    /* 디테일 패널의 헤더 부분(프로퍼티 이름이 있는 부분)을 커스터마이징합니다. */
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    /* 자식 프로퍼티들을 커스터마이징합니다. */
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

private:
    /* 속성 드롭다운 메뉴의 옵션 목록 */
    TArray<TSharedPtr<FString>> AttributePickerOptions;

    /* 현재 편집 중인 FLuxAttribute 구조체 내부의 TFieldPath 프로퍼티에 접근하기 위한 핸들 */
    TSharedPtr<IPropertyHandle> AttributeProperty;

    /* 드롭다운에서 새로운 속성이 선택되었을 때 호출될 함수 */
    void OnAttributeSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

    /* 현재 선택된 속성의 이름을 가져오는 함수 */
    FText GetCurrentAttributeName() const;
};