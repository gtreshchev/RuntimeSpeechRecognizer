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
		MinCpuArchX64 = MinimumCpuArchitectureX64.Default;
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

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "whisper.cpp"));
	}
}