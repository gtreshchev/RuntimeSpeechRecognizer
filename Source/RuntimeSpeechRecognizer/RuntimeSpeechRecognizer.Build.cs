// Georgy Treshchev 2024.

using UnrealBuildTool;
using System.IO;

public class RuntimeSpeechRecognizer : ModuleRules
{
	public RuntimeSpeechRecognizer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Enable CPU instruction sets
#if UE_5_3_OR_LATER
		// Increase to AVX2 OR AVX512 for better performance (if your CPU supports it)
		MinCpuArchX64 = MinimumCpuArchitectureX64.AVX;
#else
		bUseAVX = true;
#endif

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Core",
				"SignalProcessing",
				"AudioPlatformConfiguration"
			}
		);

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"Slate",
					"SlateCore"
				});

			if (Target.Version.MajorVersion >= 5 && Target.Version.MinorVersion >= 0)
			{
				PrivateDependencyModuleNames.AddRange(
					new string[]
					{
						"DeveloperToolSettings"
					}
				);
			}
		}

		// Add OpenBLAS support (works on Windows only for now)
		// Practically, I didn't notice any performance difference between OpenBLAS and the CPU.
		// TODO: Add support for other platforms.
		// Set to true if you want to use OpenBLAS.
		bool bUseOpenBLAS = false;
		if (bUseOpenBLAS)
		{
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PrivateDefinitions.Add("GGML_USE_BLAS");

				PrivateDependencyModuleNames.Add("Projects");

				PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "OpenBLAS", "include"));
				PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "OpenBLAS", "src"));

				PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "ggml-blas", "include"));
				PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "ggml-blas", "src"));
				
				PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "OpenBLAS", "lib", "libopenblas.lib"));
				PublicDelayLoadDLLs.Add("libopenblas.dll");
				RuntimeDependencies.Add(Path.Combine("$(PluginDir)", "Source", "ThirdParty", "OpenBLAS", "bin", "libopenblas.dll"));
			}
		}
		
		// Add Vulkan support (works on Windows only for now)
		// TODO: Fix for UE 5.5.
		// TODO: For some reason, the Vulkan acceleration slows down the recognition, while the CPU works faster, at least on my machine. Investigate.
		// TODO: Add support for other platforms.
		// Set to true if you want to use Vulkan.
		bool bUseVulkan = false;
		if (bUseVulkan)
		{
			PrivateIncludePathModuleNames.Add("VulkanRHI");
			PrivateIncludePathModuleNames.Add("Vulkan");

			string EngineSourceDirectory = Path.GetFullPath(Target.RelativeEnginePath);
			PrivateIncludePaths.Add(Path.Combine(EngineSourceDirectory, "Source/Runtime/VulkanRHI/Private"));
			AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");

			PublicIncludePaths.Add(Path.Combine(EngineSourceDirectory, "Source/ThirdParty/Vulkan/Include"));
			PrivateDefinitions.Add("GGML_USE_VULKAN");

			PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "ggml-vulkan", "include"));
			PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "ggml-vulkan", "src"));
		}

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "whisper.cpp", "include"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "whisper.cpp", "src"));

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "ggml", "include"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "ggml", "src"));
	}
}