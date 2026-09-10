// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CYB3RGUN : ModuleRules
{
	public CYB3RGUN(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags",
			"NavigationSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		if (Target.bBuildEditor)
		{
			// editor only: the console command that bootstraps state tree assets
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "StateTreeEditorModule", "AssetRegistry", "PropertyBindingUtils", "PropertyBindingUtilsEditor" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"CYB3RGUN",
			"CYB3RGUN/Variant_Horror",
			"CYB3RGUN/Variant_Horror/UI",
			"CYB3RGUN/Variant_Shooter",
			"CYB3RGUN/Variant_Shooter/AI",
			"CYB3RGUN/Variant_Shooter/UI",
			"CYB3RGUN/Variant_Shooter/Weapons",
			"CYB3RGUN/DoorRange",
			"CYB3RGUN/Enemies",
			"CYB3RGUN/Encounters",
			"CYB3RGUN/Rail"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
