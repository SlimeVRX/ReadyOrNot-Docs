"""Read native fixture table metadata without modifying assets or spawning AI."""
import json
import os
import unreal

report = {}
for path, columns in [
    ("/Game/Blueprints/DataTables/AI/AIDataTable_Training", ["CharacterClass", "SpawningTeamType", "AIBodyArmourSelection", "AIHelmetSelection", "AIBodyArmourOverride"]),
    ("/Game/Blueprints/DataTables/AIDataTable_Test", ["CharacterClass", "SpawningTeamType", "AIBodyArmourSelection", "AIHelmetSelection", "AIBodyArmourOverride"]),
    ("/Game/Blueprints/DataTables/SuspectArmourDataTable", ["bIsHelmet", "ArmourLevel", "ArmourMaterial"]),
]:
    table = unreal.load_asset(path)
    if not table:
        report[path] = {"error": "missing"}
        continue
    names = [str(n) for n in unreal.DataTableFunctionLibrary.get_data_table_row_names(table)]
    values = {col: list(unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, col)) for col in columns}
    report[path] = [{"row": name, **{key: (str(vals[i]) if i < len(vals) else "") for key, vals in values.items()}} for i, name in enumerate(names)]

registry = unreal.AssetRegistryHelpers.get_asset_registry()
report["destructible_candidates"] = []
for path in ("/Game/Blueprints/Environment/BP_GlassActor", "/Game/Blueprints/Environment/Destructible/Club/BP_Destructible_RandomBottle", "/Game/ReadyOrNot/Level/RoN_Valley/Valley_Arch/BP_BreakableGlass"):
    data = registry.get_asset_by_object_path(path + "." + path.rsplit("/", 1)[1])
    report["destructible_candidates"].append({"path": path, "native_parent": str(data.get_tag_value("NativeParentClass")), "parent": str(data.get_tag_value("ParentClass"))})
out = os.path.join(unreal.Paths.project_saved_dir(), "GunLab", "range_tables.json")
with open(out, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
unreal.log("GUNLAB_RANGE_TABLES_DONE " + out)
