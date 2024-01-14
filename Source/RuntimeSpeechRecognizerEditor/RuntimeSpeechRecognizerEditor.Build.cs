// Georgy Treshchev 2024.

using UnrealBuildTool;

public class RuntimeSpeechRecognizerEditor : ModuleRules
{
	public RuntimeSpeechRecognizerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"RuntimeSpeechRecognizer",
				"UnrealEd",
				"Projects",
				"EditorScriptingUtilities",
				"HTTP",
				"EditorStyle"
			}
		);

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
}