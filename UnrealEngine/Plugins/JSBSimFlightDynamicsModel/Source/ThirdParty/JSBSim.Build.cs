// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class JSBSim : ModuleRules
{
    public JSBSim(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        bool bJSBSimSupported = Target.Platform == UnrealTargetPlatform.Win64 ||
                                Target.Platform == UnrealTargetPlatform.Mac || 
                                Target.Platform == UnrealTargetPlatform.Linux || 
                                Target.Platform == UnrealTargetPlatform.Android;

        if (Target.Platform == UnrealTargetPlatform.Win64)
            SetupWindowsPlatform();
        else
            SetupUnixPlatform();
    }

    private void SetupWindowsPlatform()
    {
        string JSBSimLocalFolder = "JSBSim";
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
        string DllFullPath = CheckForFile(LibPath, "JSBSim.dll");
        RuntimeDependencies.Add("$(BinaryOutputDir)/" + "JSBSim.dll", DllFullPath);
    }

    private void SetupUnixPlatform()
    {
        string JSBSimLocalFolder = "JSBSim";
        string LibFolderName = "Lib";
        string LibPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, LibFolderName, $"{Target.Platform}");

        // Include headers
        string IncludePath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, "Include");
        PublicSystemIncludePaths.Add(IncludePath);

        string libExt = Target.Platform == UnrealTargetPlatform.Mac ? "dylib" : "so";
        LibPath = CheckForFile(LibPath, $"libJSBSim.{libExt}");
        
        PublicAdditionalLibraries.Add(LibPath);
        RuntimeDependencies.Add(LibPath);
        
        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "JSBSim/JSBSim_APL.xml"));
        }
    }

    private static string CheckForFile(string path, string file)
    {
        string filePath = Path.Combine(path, file);

        if (File.Exists(filePath)) 
        {
            System.Console.WriteLine($"JSBSim Path Check OK: {filePath}");
            return filePath;
        }
        
        string Err = $"{file} not found at {path}";
        System.Console.WriteLine(Err);

        throw new BuildException(Err);
    }
}
