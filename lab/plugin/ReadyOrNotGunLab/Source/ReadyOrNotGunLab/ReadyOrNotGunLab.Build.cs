using UnrealBuildTool;
using System.IO;

public class ReadyOrNotGunLab : ModuleRules
{
    public ReadyOrNotGunLab(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "ReadyOrNot", "InputCore", "Json", "JsonUtilities", "RenderCore" });
        PrivateIncludePaths.Add(Path.Combine(Target.ProjectFile.Directory.FullName, "Source", "ReadyOrNot"));
    }
}
