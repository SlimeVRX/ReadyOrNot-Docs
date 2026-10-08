"""Read-only native range dependency inspection; run in the bundled UE Python."""
import datetime
import json
import os
import re
import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), "GunLab")
os.makedirs(OUT, exist_ok=True)
report = {"schema": 1, "generated_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
          "scope": "Local asset references and authored defaults only; no gameplay mutation", "assets": [], "candidates": [], "errors": []}

def value(obj):
    if obj is None or isinstance(obj, (str, int, float, bool)):
        return obj
    if isinstance(obj, unreal.Object):
        return obj.get_path_name()
    if isinstance(obj, (list, tuple, unreal.Array)):
        return [value(x) for x in obj]
    return re.sub(r" \(0x[0-9A-Fa-f]+\)", "", str(obj))

def props(obj, names):
    result = {}
    for name in names:
        try:
            result[name] = value(obj.get_editor_property(name))
        except Exception:
            pass
    return result

fields = ["default_pawn_class", "player_controller_class", "game_state_class", "sub_pre_mission_planning_level", "ai_controller_class", "auto_possess_ai", "archetype", "archetype_data", "default_archetype", "character_data", "character_data_table", "mesh", "max_health", "popup_time", "health", "b_fall_down", "door_type", "door_data", "door_data_table", "door_name", "b_locked", "b_can_player_interact", "b_is_destructible", "target_mesh", "success_box", "failure_box", "interactable_component", "surface_type", "phys_material", "physical_material", "physical_material_override", "sound_data", "target", "target_class", "target_distance", "b_open_customization", "b_cannot_take_damage"]

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
native_types = ("LoadoutPortal", "AmmoRefillBox", "Door", "PopupTarget", "TrainingTarget", "SuspectCharacter", "CivilianCharacter")
for asset in registry.get_assets_by_path("/Game", recursive=True):
    path = str(asset.package_name)
    parent = str(asset.get_tag_value("NativeParentClass"))
    name = str(asset.asset_name)
    if (any(parent.endswith("." + t + "'") for t in native_types)
            or name in ("BP_TargetBase", "BP_ShootingSetup", "BP_Station_Interact_ShootingRange_Rails_01", "BP_HQ_Target_01")
            or path.startswith("/Game/Blueprints/AI/Archetypes/Debug/Debug_Suspect_Static")
            or path.startswith("/Game/ReadyOrNot/Data/Phys-Materials/")
            or ("Door" in name and "Blueprints/Actors" in path)):
        report["candidates"].append({"path": path, "class": str(asset.asset_class_path.asset_name), "native_parent": parent, "parent": str(asset.get_tag_value("ParentClass"))})

selected = ["/Game/Blueprints/Games/GM_FreeMode", "/Game/Blueprints/Games/GS_FreeMode",
            "/Game/Blueprints/Actors/ShootingRange/BP_TargetBase",
            "/Game/ReadyOrNot/Shoothouse/BP_ShoothouseTarget",
            "/Game/ReadyOrNot/Shoothouse/BP_TrainingTarget",
            "/Game/ReadyOrNot/Level/RoN_Station/HQ_Killhouse_Trainingroom/BP_HQ_Target_01",
            "/Game/ReadyOrNot/Level/Dev/Level_Design/Blueprints/BP_AmmoRefillBox_01",
            "/Game/Blueprints/Characters/AI/CyberneticsSuspect_V2",
            "/Game/Blueprints/AI/Archetypes/Debug/Debug_Suspect_Static",
            "/Game/Blueprints/AI/Archetypes/Debug/Debug_Civilian_Static",
            "/Game/ReadyOrNot/Level/RoN_Station/Killhouse_Decor/ShootingRange/Target/BP_ShootingSetup",
            "/Game/Blueprints/Environment/Destructible/Interactable/BP_Station_Interact_ShootingRange_Rails_01"]
selected += [x["path"] for x in report["candidates"] if x["native_parent"].endswith(".Door'")][:5]
selected += ["/Game/ReadyOrNot/Data/Phys-Materials/RON_" + x + "_PM" for x in ("Drywall", "Wood_Soft", "Wood_Hard", "Steel", "Concrete_Strong", "Glass_Plate", "Aluminium")]
selected += ["/Script/ReadyOrNot." + x for x in ("LoadoutPortal", "AmmoRefillBox", "Door", "ReadyOrNotGameState", "SuspectCharacter", "AIArchetypeData")]
for path in selected:
    try:
        obj = unreal.load_class(None, path) if path.startswith("/Script/") else unreal.load_asset(path)
        if isinstance(obj, unreal.Blueprint):
            obj = unreal.EditorAssetLibrary.load_blueprint_class(path)
        if isinstance(obj, unreal.Class):
            obj = unreal.get_default_object(obj)
        assert obj, "Object not found"
        entry = {"path": path, "object_class": obj.get_class().get_path_name(), "values": props(obj, fields)}
        # Reflection names are metadata; record selected exposed properties for archetypes.
        if "Archetype" in path:
            entry["python_members"] = [x for x in dir(obj) if any(s in x for s in ("character", "mesh", "armor", "armour", "health", "behavior", "behaviour", "activity", "loadout", "weapon"))]
        if isinstance(obj, unreal.Actor):
            entry["components"] = [{"name": c.get_name(), "class": c.get_class().get_path_name(), "values": props(c, ["static_mesh", "skeletal_mesh", "anim_class", "relative_location", "relative_rotation", "collision_profile_name", "phys_material_override", "show_prompt_at_distance"])} for c in obj.get_components_by_class(unreal.ActorComponent)]
        if path.endswith("Door"):
            entry["python_members"] = [x for x in dir(obj) if any(s in x for s in ("door", "lock", "row", "data"))]
        report["assets"].append(entry)
    except Exception as exc:
        report["errors"].append({"path": path, "error": str(exc)})

door_table = unreal.load_asset("/Game/Blueprints/DataTables/DoorDataTable")
if door_table:
    report["door_rows"] = [str(x) for x in unreal.DataTableFunctionLibrary.get_data_table_row_names(door_table)]

with open(os.path.join(OUT, "range_dependencies.json"), "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2, ensure_ascii=False)
unreal.log("GUNLAB_RANGE_DEPENDENCIES_DONE assets=%d candidates=%d errors=%d" % (len(report["assets"]), len(report["candidates"]), len(report["errors"])))
