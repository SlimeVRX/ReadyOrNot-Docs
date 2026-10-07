"""Run only inside the matching ReadyOrNot Unreal Editor Python environment.

Reads local assets; exports references and selected authored values, not asset bytes.
"""
import json
import os
import traceback
import re
from collections import Counter
import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), "GunLab")
os.makedirs(OUT, exist_ok=True)

def value(obj):
    if obj is None or isinstance(obj, (str, int, float, bool)):
        return obj
    if isinstance(obj, unreal.Object):
        return obj.get_path_name()
    if isinstance(obj, unreal.Vector):
        return {"x": obj.x, "y": obj.y, "z": obj.z}
    if isinstance(obj, unreal.Rotator):
        return {"pitch": obj.pitch, "yaw": obj.yaw, "roll": obj.roll}
    if isinstance(obj, (list, tuple, unreal.Array)):
        return [value(x) for x in obj]
    return re.sub(r" \(0x[0-9A-Fa-f]+\)", "", str(obj))

def properties(obj, names):
    out = {}
    for name in names:
        try:
            out[name] = value(obj.get_editor_property(name))
        except Exception:
            pass
    return out

report = {"schema": 1, "engine": unreal.SystemLibrary.get_engine_version(), "weapons": [], "modes": [], "errors": []}
try:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    all_assets = registry.get_assets_by_path("/Game", recursive=True)
    report["asset_class_counts"] = dict(Counter(str(x.asset_class_path.asset_name) for x in all_assets))
    report["game_asset_count"] = len(all_assets)
    # Check native parent metadata across /Game, including guns outside the
    # expected Items folders. Cache native CDO checks; do not load other BPs.
    native_parent_cache = {}
    assets = []
    for data in all_assets:
        if str(data.asset_class_path.asset_name) != "Blueprint":
            continue
        native = str(data.get_tag_value("NativeParentClass"))
        if native not in native_parent_cache:
            try:
                native_path = native.split("'")[1] if "'" in native else native
                native_cls = unreal.load_class(None, native_path)
                native_parent_cache[native] = bool(native_cls and isinstance(unreal.get_default_object(native_cls), unreal.BaseMagazineWeapon))
            except Exception:
                native_parent_cache[native] = False
        if native_parent_cache[native]:
            assets.append(data)
    report["registry_candidate_count"] = len(assets)
    for data in assets:
        if str(data.asset_class_path.asset_name) != "Blueprint":
            continue
        path = str(data.package_name)
        try:
            cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
            cdo = unreal.get_default_object(cls) if cls else None
            if not isinstance(cdo, unreal.BaseMagazineWeapon):
                continue
            item = {"asset_path": path, "class_path": cls.get_path_name(), "class_name": cls.get_name(),
                    "parent_class": str(data.get_tag_value("ParentClass")),
                    "native_parent": str(data.get_tag_value("NativeParentClass")),
                    "spawnable_candidate": True,
                    "values": properties(cdo, ["item_name", "item_category", "item_class", "animation_data", "sound_data", "ammo_data_table", "ammunition_types", "ammo_max", "mag_count", "current_fire_mode", "available_fire_modes", "fire_rate", "cartridge_text", "rpm_text", "capacity_text", "muzzle_velocity_text", "recoil_pattern", "recoil_return_rate", "ads_recoil_multiplier", "ads_spread_multiplier", "first_shot_recoil", "first_shot_spread", "recoil_fire_strength", "recoil_fire_strength_first", "recoil_angle_strength", "recoil_randomness", "recoil_fire_ads_modifier", "recoil_angle_ads_modifier", "recoil_rotation_buildup", "recoil_position_buildup", "recoil_buildup_ads_modifier", "velocity_spread_multiplier", "velocity_recoil_multiplier", "spread_pattern", "scope_attachment", "muzzle_attachment", "underbarrel_attachment"])}
            report["weapons"].append(item)
            unreal.log("GUNLAB_CATALOG " + cls.get_name())
        except Exception as exc:
            report["errors"].append({"asset_path": path, "error": str(exc)})
    for path in ["/Game/Blueprints/Games/GM_FreeMode", "/Game/Blueprints/Games/GM_Training", "/Game/Blueprints/Games/GM_TestGameMode_Simulate", "/Game/Blueprints/Games/GM_Base", "/Game/Blueprints/Characters/BasePlayer"]:
        try:
            cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
            cdo = unreal.get_default_object(cls)
            report["modes"].append({"asset_path": path, "class_path": cls.get_path_name(), "values": properties(cdo, ["default_pawn_class", "player_controller_class", "player_state_class", "game_state_class", "hud_class", "blue_character_class", "red_character_class", "b_initial_player_respawn", "b_run_warmup", "game_mode_settings", "b_can_respawn"] )})
        except Exception as exc:
            report["errors"].append({"asset_path": path, "error": str(exc)})
except Exception:
    report["errors"].append({"fatal": traceback.format_exc()})
finally:
    report["weapons"].sort(key=lambda x: x["class_path"])
    parents = Counter((x["parent_class"].split("'")[1] if "'" in x["parent_class"] else x["parent_class"]) for x in report["weapons"])
    for item in report["weapons"]:
        item["blueprint_child_count"] = parents.get(item["class_path"], 0)
        item["leaf_blueprint"] = item["blueprint_child_count"] == 0
    with open(os.path.join(OUT, "native_inspection.json"), "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2, ensure_ascii=False)
    unreal.log("GUNLAB_INSPECT_DONE count=" + str(len(report["weapons"])))
