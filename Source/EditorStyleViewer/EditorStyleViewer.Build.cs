// Copyright (C) Vasily Bulgakov. 2024. All Rights Reserved.

using UnrealBuildTool;

public class EditorStyleViewer : ModuleRules
{
	public EditorStyleViewer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore", 
				"WorkspaceMenuStructure",
				"ApplicationCore",
			}
			);
	}
}
