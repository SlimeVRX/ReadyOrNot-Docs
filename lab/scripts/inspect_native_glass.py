"""Inspect original glass Blueprint signatures and dependencies; no world edits."""
import json
import os
import unreal

paths = ["/Game/ThirdParty/ProceduralGlass/Blueprints/BP_ProceduralDestructibleGlass",
         "/Game/ReadyOrNot/Level/RoN_Valley/Valley_Arch/BP_BreakableGlass",
         "/Game/ThirdParty/BreakableGlass/Blueprints/BP_BreakableGlass",
         "/Game/ThirdParty/BreakableGlass/Blueprints/BP_BreakableGlass_v01"]
report = []
for path in paths:
    row = {"path": path, "methods": {}, "properties": {}}
    try:
        cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
        obj = unreal.get_default_object(cls)
        row["class"] = cls.get_path_name()
        row["members"] = [n for n in dir(obj) if any(t in n.lower() for t in ("hit", "damage", "cut", "glass", "mesh", "force", "size", "physics"))]
        for name in row["members"]:
            member = getattr(obj, name, None)
            if callable(member):
                row["methods"][name] = str(getattr(member, "__doc__", ""))
            else:
                try:
                    value = obj.get_editor_property(name)
                    row["properties"][name] = value.get_path_name() if isinstance(value, unreal.Object) else str(value)
                except Exception:
                    pass
    except Exception as exc:
        row["error"] = str(exc)
    report.append(row)
out = os.path.join(unreal.Paths.project_saved_dir(), "GunLab", "native_glass_inspection.json")
with open(out, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
unreal.log("GUNLAB_NATIVE_GLASS_INSPECT_DONE " + out)
