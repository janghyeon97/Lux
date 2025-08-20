// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Lux : ModuleRules
{
	public Lux(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				"Lux"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NavigationSystem",
			"AIModule", "" +
			"UMG",
			"Slate",
			"SlateCore",
			"ModularGameplay",
			"PhysicsCore",
			"ModularGameplayActors",
			"StructUtils"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"NetCore"
		});
	}
}
