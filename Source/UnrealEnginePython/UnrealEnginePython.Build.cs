// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System.Collections.Generic;

public class UnrealEnginePython : ModuleRules
{
    private string PythonHome;

#if WITH_FORWARDED_MODULE_RULES_CTOR
    public UnrealEnginePython(ReadOnlyTargetRules Target) : base(Target)
#else
    public UnrealEnginePython(TargetInfo Target)
#endif
    {

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDefinitions.Add("WITH_UNREALENGINEPYTHON=1"); // fixed
        PythonHome = Path.Combine(PluginDirectory, "Resources", "python_build_dependency");
        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Sockets",
                "Networking"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "MovieScene",
                "LevelSequence",
                "HTTP",
                "UMG",
                "AppFramework",
                "RHI",
                "Voice",
                "RenderCore",
                "MovieSceneCapture",
                "Landscape",
                "Foliage",
                "AIModule",
                "ApplicationCore",
                "HairStrandsCore"
				// ... add private dependencies that you statically link with here ...
			}
            );

#if WITH_FORWARDED_MODULE_RULES_CTOR
        BuildVersion Version;
        if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
        {
            if (Version.MinorVersion >= 18)
            {
                PrivateDependencyModuleNames.Add("ApplicationCore");
            }
        }
#endif


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );

#if WITH_FORWARDED_MODULE_RULES_CTOR
        if (Target.bBuildEditor)
#else
        if (UEBuildConfiguration.bBuildEditor)
#endif
        {
            PrivateDependencyModuleNames.AddRange(new string[]{
                "UnrealEd",
                "LevelEditor",
                "BlueprintGraph",
                "Projects",
                "Sequencer",
                "SequencerWidgets",
                "AssetTools",
                "LevelSequenceEditor",
                "MovieSceneTools",
                "MovieSceneTracks",
                "CinematicCamera",
                "EditorStyle",
                "GraphEditor",
                "UMGEditor",
                "AIGraph",
                "RawMesh",
                "DesktopWidgets",
                "EditorWidgets",
                "FBX",
                "Persona",
                "PropertyEditor",
                "LandscapeEditor",
                "MaterialEditor",
                "Json",
                "AssetManagerEditor"
            });
        }

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            SetPath(PythonHome);
            System.Console.WriteLine("[UnrealEnginePython] Using Python at: " + PythonHome);
            PublicIncludePaths.Add(PythonHome);
            string libPath = GetWindowsPythonLibFile(PythonHome);
            System.Console.WriteLine("[UnrealEnginePython] Using libPath at: " + libPath);
            PublicAdditionalLibraries.Add(libPath);

            string PydllPath = Path.GetFullPath(Path.Combine(PluginDirectory, "Resources", "python_runtime_dependency"));
            foreach (string FilePath in Directory.EnumerateFiles(PydllPath, "*", SearchOption.AllDirectories))
            {
                if (FilePath.EndsWith(".dll") || FilePath.EndsWith(".pyd") || FilePath.EndsWith(".zip"))
                {
                    string FileName = Path.GetFileName(FilePath);
                    RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", FileName), FilePath);
                }
            }
        }
        else
        {
            throw new System.Exception(string.Format("[UnrealEnginePython] Unsupported Platform: {0}, Only support Win64.", Target.Platform.ToString()));
        }
    }

    private static bool IsPathRelative(string Path)
    {
        bool IsRooted = Path.StartsWith("\\", System.StringComparison.Ordinal) || // Root of the current directory on Windows. Also covers "\\" for UNC or "network" paths.
                        Path.StartsWith("/", System.StringComparison.Ordinal) ||  // Root of the current directory on Windows, root on UNIX-likes.
                                                                                  // Also covers "\\", considering normalization replaces "\\" with "//".
                        (Path.Length >= 2 && char.IsLetter(Path[0]) && Path[1] == ':'); // Starts with "<DriveLetter>:"
        return !IsRooted;
    }

    private string DiscoverPythonPath(string[] knownPaths, string binaryPath)
    {
        List<string> paths = new List<string>(knownPaths);
        // insert the PYTHONHOME content
        string environmentPath = System.Environment.GetEnvironmentVariable("PYTHONHOME");
        if (!string.IsNullOrEmpty(environmentPath))
            paths.Add(environmentPath);

        // look in an alternate custom location
        environmentPath = System.Environment.GetEnvironmentVariable("UNREALENGINEPYTHONHOME");
        if (!string.IsNullOrEmpty(environmentPath))
            paths.Add(environmentPath);

        foreach (string path in paths)
        {
            string actualPath = path;

            if (IsPathRelative(actualPath))
            {
                actualPath = Path.GetFullPath(Path.Combine(ModuleDirectory, actualPath));
            }

            string headerFile = Path.Combine(actualPath, "include", "Python.h");
            if (File.Exists(headerFile))
            {
                return actualPath;
            }
            // this is mainly useful for OSX
            headerFile = Path.Combine(actualPath, "Headers", "Python.h");
            if (File.Exists(headerFile))
            {
                return actualPath;
            }
        }
        return "";
    }

    private static string GetWindowsPythonLibFile(string basePath)
    {
        // try with python3
        for (int i = 9; i >= 0; i--)
        {
            string fileName = string.Format("python3{0}.lib", i);
            string fullPath = Path.Combine(basePath, "libs", fileName);
            if (File.Exists(fullPath))
            {
                return fullPath;
            }
        }
        throw new System.Exception("[UnrealEnginePython] Invalid Python installation, missing .lib files");
    }

    private static bool CopyDirectory(string SourcePath, string DestinationPath, bool overwriteexisting)
    {
        bool ret = false;

        SourcePath = SourcePath.EndsWith(@"\") ? SourcePath : SourcePath + @"\";
        DestinationPath = DestinationPath.EndsWith(@"\") ? DestinationPath : DestinationPath + @"\";

        if (Directory.Exists(SourcePath))
        {
            if (Directory.Exists(DestinationPath) == false)
                Directory.CreateDirectory(DestinationPath);

            foreach (string fls in Directory.GetFiles(SourcePath))
            {
                FileInfo flinfo = new FileInfo(fls);
                if (!File.Exists(DestinationPath + flinfo.Name))
                {
                    flinfo.CopyTo(DestinationPath + flinfo.Name, overwriteexisting);
                }
            }
            foreach (string drs in Directory.GetDirectories(SourcePath))
            {
                DirectoryInfo drinfo = new DirectoryInfo(drs);
                if (CopyDirectory(drs, DestinationPath + drinfo.Name, overwriteexisting) == false)
                    ret = false;
            }
        }
        ret = true;

        return ret;
    }

    public static void SetPath(string pathValue)
    {
        string pathstr;
        pathstr = System.Environment.GetEnvironmentVariable("PATH");
        bool isContains = pathstr.ToLower().Contains(pathValue.ToLower());//true
        if (!isContains)
        {
            System.Environment.SetEnvironmentVariable("PATH", pathstr + pathValue + ";");
        }
        return;
    }
}
