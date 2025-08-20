// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "LuxAttributeCustomization.h"
#include "ActionSystem/Attributes/LuxAttributeSet.h" 

class FLuxEditorModule : public IModuleInterface
{

public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyModule.RegisterCustomPropertyTypeLayout(
			FLuxAttribute::StaticStruct()->GetFName(),
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLuxAttributeCustomization::MakeInstance)
		);
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomPropertyTypeLayout(FLuxAttribute::StaticStruct()->GetFName());
		}
	}
};

IMPLEMENT_MODULE(FLuxEditorModule, LuxEditor);