// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class JSBSim : ModuleRules
{
	public JSBSim(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		bool bSupported = Target.Platform == UnrealTargetPlatform.Win64 ||
			Target.Platform == UnrealTargetPlatform.Mac ||
			Target.Platform == UnrealTargetPlatform.Linux; // Android Soon
			
		if (!bSupported) return;
		
		// This folder is where the JSBSimForUnreal build has been generated
		string JSBSimLocalFolder = "JSBSim";
		string LibFolderName = "Lib";
		string LibPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, LibFolderName);
				
		// Include headers
		string IncludePath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, "Include");
		System.Console.WriteLine($"JSBSim Include Path: {IncludePath}");
		PublicSystemIncludePaths.Add(IncludePath);
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
            // When working in debug mode, try to use the Debug version of JSBSim
            if (Target.Configuration == UnrealTargetConfiguration.Debug)
            {
                // Source\ThirdParty\JSBSim\LibDebug
                string DebugLibsPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, LibFolderName + "Debug");
                if (Directory.Exists(DebugLibsPath))
                {
                    System.Console.WriteLine(string.Format("Found Debug libraries for JSBSim in {0}", DebugLibsPath));
                    LibFolderName += "Debug";
                }
            }
        		
			// Link Lib
			PublicAdditionalLibraries.Add(Path.Combine(LibPath, "JSBSim.lib"));
			
			// Stage DLL along the binaries files
			string DllFullPath = Path.Combine(LibPath, "JSBSim.dll");
			if (!File.Exists(DllFullPath))
            {
			    string Err = string.Format("JSBSim.dll not found in {0} - Make sure that you have built JSBSimForUnreal.sln first", LibPath);
                System.Console.WriteLine(Err);
                throw new BuildException(Err);
            }
            RuntimeDependencies.Add("$(BinaryOutputDir)/" + "JSBSim.dll", DllFullPath);
		}
		else 
		{
			// See https://forums.unrealengine.com/t/busertti-true-makes-an-unreal-project-fail-to-load/407837
			bUseRTTI = true;
			bEnableExceptions = true;
			
			LibPath = Path.Combine(LibPath, $"{Target.Platform}");
			System.Console.WriteLine($"JSBSim Lib Path: {LibPath}");
			// Same for Linux, Macos and Android
			//PublicLibraryPaths.Add(LibPath);
			PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libJSBSim.a"));
		}
	}
}
