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
			"NavigationSystem",
			"RHI",
			"RenderCore",
			"Niagara",
			"DeveloperSettings",
			"EngineSettings"
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
			"CYB3RGUN/Rail",
			"CYB3RGUN/Settings",
			"CYB3RGUN/UI",
			"CYB3RGUN/Benchmark",
			"CYB3RGUN/VFX",
			"CYB3RGUN/Menu",
			"CYB3RGUN/Style",
			"CYB3RGUN/Weapons",
			"CYB3RGUN/Feel",
			"CYB3RGUN/Combat"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
