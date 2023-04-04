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

    private void SetupUnixPlatform()
    {
        string JSBSimLocalFolder = "JSBSim";
        string LibFolderName = "Lib";
        string LibPath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, LibFolderName);

        // Include headers
        string IncludePath = Path.Combine(ModuleDirectory, JSBSimLocalFolder, "Include");
        System.Console.WriteLine($"JSBSim Include Path: {IncludePath}");
        PublicIncludePaths.Add(IncludePath);

        //bUseRTTI = true;
        //bEnableExceptions = true;

        LibPath = Path.Combine(LibPath, $"{Target.Platform}");
        System.Console.WriteLine($"JSBSim Lib Path: {LibPath}");
        PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libJSBSim.a"));

        string DllFullPath = Path.Combine(LibPath, "libJSBSim.so");

        if (File.Exists(DllFullPath))
        {
            RuntimeDependencies.Add("$(BinaryOutputDir)/" + "libJSBSim.so", DllFullPath);
        }
        else
        {
            System.Console.WriteLine("libJSBSim.so not found");
        }
    }
}
