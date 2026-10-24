// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CMBGameJam : ModuleRules
{
	public CMBGameJam(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CMBGameJam",
			"CMBGameJam/Variant_Horror",
			"CMBGameJam/Variant_Horror/UI",
			"CMBGameJam/Variant_Shooter",
			"CMBGameJam/Variant_Shooter/AI",
			"CMBGameJam/Variant_Shooter/UI",
			"CMBGameJam/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
