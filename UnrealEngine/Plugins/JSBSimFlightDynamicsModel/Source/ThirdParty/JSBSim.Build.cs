// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class JSBSim : ModuleRules
{
	public JSBSim(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		
		// This folder is where the JSBSimForUnreal build has been generated
		string JSBSimLocalFolder = "JSBSim";

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string LibFolderName = "Lib";

            // When working in debug mode, try to use the Debug version of JSBSim
            if (Target.Configuration == UnrealTargetConfiguration.DebugGame)
            {
				// Source\ThirdParty\JSBSim\LibDebug
				string DebugLibsPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, LibFolderName + "Debug");
                if (Directory.Exists(DebugLibsPath))
                {
					System.Console.WriteLine(string.Format("Found Debug libraries for JSBSim in {0}", DebugLibsPath));
					LibFolderName += "Debug";
				}
			}

			// Include headers
			PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, JSBSimLocalFolder, "Include"));

			// Link Lib
			string LibPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, LibFolderName);
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
	}
}
