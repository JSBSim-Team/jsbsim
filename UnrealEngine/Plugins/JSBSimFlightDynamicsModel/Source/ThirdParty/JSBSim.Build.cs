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
        string DllFullPath = CheckForFile(LibPath, "JSBSim.dll");
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
        PublicSystemIncludePaths.Add(IncludePath);

        bUseRTTI = true;
        bEnableExceptions = true;

        LibPath = Path.Combine(LibPath, $"{Target.Platform}");
        System.Console.WriteLine($"JSBSim Lib Path: {LibPath}");

        string staticLibPath = CheckForFile(LibPath, "libJSBSim.a");
        PublicAdditionalLibraries.Add(staticLibPath);

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string macFrameworkPath = CheckForDirectory(LibPath, "JSBSim.framework");
            string macJSBSimExec = CheckForFile(LibPath, "JSBSim");

            //RuntimeDependencies.Add("$(BinaryOutputDir)/" + "JSBSim.framework", macFrameworkPath);
            RuntimeDependencies.Add("$(BinaryOutputDir)/" + "JSBSim", macJSBSimExec);  
        }
        else 
        {
            string soFilePath = CheckForFile(LibPath, "libJSBSim.so");
            RuntimeDependencies.Add("$(BinaryOutputDir)/" + "libJSBSim.so", soFilePath);
        }
    }

    private static string CheckForFile(string path, string file)
    {
        string filePath = Path.Combine(path, file);
        if (File.Exists(filePath))
            return filePath;

        string Err = $"{file} not found at {path}";
        System.Console.WriteLine(Err);
        throw new BuildException(Err);
    }

    private static string CheckForDirectory(string path, string directory)
    {
        string directoryPath = Path.Combine(path, directory);
        if (Directory.Exists(directoryPath))
            return directoryPath;

        string Err = $"{directory} not found at {path}";
        System.Console.WriteLine(Err);
        throw new BuildException(Err);
    }
}
