// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class JSBSim : ModuleRules
{
	public JSBSim(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		bUseRTTI = true;

		bool bSupported = Target.Platform == UnrealTargetPlatform.Win64 ||
			Target.Platform == UnrealTargetPlatform.Mac ||
			Target.Platform == UnrealTargetPlatform.Linux; // Android Soon
			
		if (!bSupported) return;
		
		// This folder is where the JSBSimForUnreal build has been generated
		string JSBSimLocalFolder = "JSBSim";
		string LibFolderName = "Lib";
		string LibPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, LibFolderName);
				
		// Include headers
		PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, JSBSimLocalFolder, "Include"));

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
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(Path.Combine(LibPath, "JSBSim.Framework"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
		    PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libJSBSim.so"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			LibPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, Path.Combine(LibFolderName, "Android"));
		    PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libJSBSim.so"));
		}
	}
}
