using UnrealBuildTool;
using System.IO;

public class ReadyOrNotGunLab : ModuleRules
{
    public ReadyOrNotGunLab(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "ReadyOrNot", "InputCore", "Json", "JsonUtilities", "RenderCore", "Slate", "SlateCore", "GameplayCameras", "TemplateSequence", "MovieScene", "MovieSceneTracks" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.Add("UnrealEd");
        PrivateIncludePaths.Add(Path.Combine(Target.ProjectFile.Directory.FullName, "Source", "ReadyOrNot"));
    }
}
