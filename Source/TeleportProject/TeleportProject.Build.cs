// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TeleportProject : ModuleRules
{
	public TeleportProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay",
			"OnlineSubSystem", "OnlineSubSystemNull", "OnlineSubsystemSteam" });

		//PrivateDependencyModuleNames.AddRange(new string[] {"OnlineSubSystem", "OnlineSubSystemNull", "OnlineSubsystemSteam"});
	}
}