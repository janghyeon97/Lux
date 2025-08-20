// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LuxEditor : ModuleRules
{
	public LuxEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				"LuxEditor"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);

		PublicDependencyModuleNames.AddRange(new string[] 
		{
			"Core", 
			"CoreUObject", 
			"Engine" 
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Lux",
            "Slate",
			"SlateCore",
			"UnrealEd",
			"PropertyEditor",
			"InputCore"
		});
	}
}
