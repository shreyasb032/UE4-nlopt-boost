// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class UE4_nloptLibrary : ModuleRules
{
	public UE4_nloptLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

        // Add the import library
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "nlopt.lib"));
		
		// Add include paths
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
		PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

		// Delay-load the DLL, so we can load it from the right place first
		PublicDelayLoadDLLs.Add("nlopt.dll");

		// Ensure that the DLL is staged along with the executable
		RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/UE4_nloptLibrary/Win64/nlopt.dll");
	}
}
