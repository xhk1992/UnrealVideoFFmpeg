// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class VideoStream : ModuleRules
{

	public VideoStream(ReadOnlyTargetRules Target) : base(Target)
	{
        //Type = ModuleType.External;
        OptimizeCode = CodeOptimization.Never;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;
        //bEnableUndefinedIdentifierWarnings = false;
        //bEnableExceptions = true;
        //PublicDefinitions.Add("UE4_LINUX_USE_LIBCXX=0");

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
                "OpenCVHelper",
                "OpenCV",
                "UFFmpeg",
                "UMuxer",
                "RHI",
                "MovieSceneCapture",
                "RenderCore", 
				"Renderer", 
                "AudioMixer",
                "PixelStreaming",
                "PixelCapture",
                "HTTP",
                "AVEncoder"
				// ... add other public dependencies that you statically link with here ...
			}
			);;
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
				"Engine",
				"Slate", 
				"SlateCore",
                "MovieSceneCapture",
                "RenderCore",
                "Renderer",
                "PixelCapture",
                "RHI"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
        PublicDelayLoadDLLs.Add("mfplat.dll");
        PublicDelayLoadDLLs.Add("mfuuid.dll");
        PublicDelayLoadDLLs.Add("Mfreadwrite.dll");
    }
}
