// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class UFFmpeg : ModuleRules
{

	private string ThirdPartyPath
	{
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty")); }
	}

	public UFFmpeg(ReadOnlyTargetRules Target) : base(Target)
	{
        //Type = ModuleType.External;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDefinitions.Add("AV_VERBOSE= 0");

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "ImageWrapper", "RenderCore", "Renderer", "RHI", "JsonUtilities", "Json", "HTTPServer", "MediaAssets","MovieSceneCapture", "Slate", "SlateCore","OpenCVHelper",
                "OpenCV"});

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            System.Console.WriteLine("... ThirdPartyPath -> " + ThirdPartyPath);
            string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "Win32";
            string LibrariesPath = Path.Combine(ThirdPartyPath, "ffmpeg", "lib", "vs", PlatformString);

            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avcodec.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avdevice.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avfilter.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avformat.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avutil.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "swresample.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "swscale.lib"));

            string[] dlls = { "avcodec-58.dll", "avdevice-58.dll", "avfilter-7.dll", "avformat-58.dll", "avutil-56.dll", "swresample-3.dll", "swscale-5.dll", "postproc-55.dll" };

            string BinariesPath = Path.Combine(ThirdPartyPath, "ffmpeg", "bin", "vs", PlatformString);
            System.Console.WriteLine("... LibrariesPath -> " + BinariesPath);
            
            string DllTargetDir = "$(ProjectDir)/Binaries/Win64/";
                
            foreach (string dll in dlls)
            {
                PublicDelayLoadDLLs.Add(dll);
                
                if (!Target.bBuildEditor)
                {
                    RuntimeDependencies.Add(Path.Combine(DllTargetDir, dll), Path.Combine(BinariesPath, dll));
                }
                else
                {
                    RuntimeDependencies.Add(Path.Combine(BinariesPath, dll));
                }
            }

            
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string LibrariesPath = Path.Combine(Path.Combine(ThirdPartyPath, "ffmpeg", "lib"), "osx");

            System.Console.WriteLine("... LibrariesPath -> " + LibrariesPath);

            string[] libs = { "libavcodec.58.dylib", "libavdevice.58.dylib", "libavfilter.7.dylib", "libavformat.58.dylib", "libavutil.56.dylib", "libswresample.3.dylib", "libswscale.5.dylib", "libpostproc.55.dylib" };
            foreach (string lib in libs)
            {
                PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, lib));
                RuntimeDependencies.Add(Path.Combine(LibrariesPath, lib), StagedFileType.NonUFS);
            }

        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string LibrariesPath = Path.Combine(Path.Combine(ThirdPartyPath, "ffmpeg", "lib"), "linux");

            System.Console.WriteLine("... LibrariesPath -> " + LibrariesPath);

            //string[] libs = { "libavcodec.so", "libavdevice.so", "libavfilter.so", "libavformat.so", "libavutil.so", "libswresample.so", "libswscale.so", "libpostproc.so" };

            string[] libs = { "libavcodec.a", "libavdevice.a", "libavfilter.a", "libavformat.a", "libavutil.a", "libswresample.a", "libswscale.a", "libpostproc.a","libx264.a" };
            //string[] copyLibs = { "libswresample.so.3", "libswscale.so.5", "libavfilter.so.7", "libpostproc.so.55", "libavutil.so.56", "libavcodec.so.58", "libavdevice.so.58", "libavformat.so.58", /*"libnppc.so.10", "libnppicc.so.10", "libnppidei.so.10", "libnppig.so.10"*/
            //};
            foreach (string lib in libs)
            {
                PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, lib));
                RuntimeDependencies.Add(Path.Combine(LibrariesPath, lib), StagedFileType.NonUFS);
            }

        }
        // Include path
        PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "ffmpeg", "include"));
        //System.Console.WriteLine("##################" + Directory.GetCurrentDirectory());
        //PrivateDependencyModuleNames.AddRange(new string[] { "MovieSceneCapture", "Slate", "SlateCore", "Engine", "CoreUObject", "RHI", "RenderCore", "Projects","UnrealEd"});
        PrivateDependencyModuleNames.AddRange(new string[] { "MovieSceneCapture", "Slate", "SlateCore", "Engine", "CoreUObject", "RHI", "RenderCore", "Projects" });

        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
        }

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        //bEnableUndefinedIdentifierWarnings = false;

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}
