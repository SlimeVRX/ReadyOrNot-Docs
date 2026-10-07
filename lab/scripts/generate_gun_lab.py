"""Generate an additive local .umap; never copies proprietary assets into the docs.

Run with UnrealEditor-Cmd <project> -run=pythonscript -script=<this file>.
Requires the compiled ReadyOrNotGunLab plugin and native_inspection.json.
"""
import json
import os
import runpy
import unreal

MAP = "/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab"
ROOT = os.path.join(unreal.Paths.project_saved_dir(), "GunLab")
if not os.path.exists(os.path.join(ROOT, "native_inspection.json")):
    runpy.run_path(os.path.join(os.path.dirname(__file__), "inspect_native.py"))
with open(os.path.join(ROOT, "native_inspection.json"), encoding="utf-8") as handle:
    catalog = json.load(handle)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
nonmagazine = []
for asset in registry.get_assets_by_path("/Game", recursive=True):
    if str(asset.asset_class_path.asset_name) != "Blueprint":
        continue
    native = str(asset.get_tag_value("NativeParentClass"))
    if native.endswith("/Script/ReadyOrNot.BaseWeapon'") or native.endswith("/Script/ReadyOrNot.MeleeWeapon'"):
        nonmagazine.append({"asset_path": str(asset.package_name), "native_parent": native,
                            "classification": "melee" if "MeleeWeapon" in native else "direct_base_weapon_requires_review"})
with open(os.path.join(ROOT, "coverage_receipt.json"), "w", encoding="utf-8") as handle:
    json.dump({"scope": "/Game Blueprint NativeParentClass tags", "magazine_candidates": len(catalog["weapons"]), "other_weapon_blueprints": nonmagazine}, handle, indent=2)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not unreal.EditorAssetLibrary.does_asset_exist(MAP), "Map exists; back it up or deliberately remove only this generated map before regenerating."
assert levels.new_level(MAP), "Failed creating map"
world = unreal.EditorLevelLibrary.get_editor_world()
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
assert cube

def box(name, location, scale):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    actor.set_actor_label(name)
    component = actor.static_mesh_component
    component.set_static_mesh(cube)
    component.set_collision_profile_name("BlockAll")
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

def text(label, location, size=42):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*location), unreal.Rotator(pitch=0, yaw=0 if location[0] < 0 else 180, roll=0))
    actor.set_actor_label(label)
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(label)
    component.set_world_size(size)
    component.set_text_render_color(unreal.Color(r=25, g=180, b=220, a=255))
    return actor

box("Range floor 130m", (5500, 0, -25), (130, 32, 0.5))
box("Backstop", (11800, 0, 450), (0.8, 32, 9))
box("Left containment", (5500, -1600, 250), (130, 0.4, 5))
box("Right containment", (5500, 1600, 250), (130, 0.4, 5))
for i, distance in enumerate([5, 10, 25, 50, 100]):
    y = (i - 2) * 550
    x = distance * 100
    box("Target %dm" % distance, (x, y, 150), (0.10, 1.0, 1.8))
    box("Target center %dm" % distance, (x - 7, y, 150), (0.04, 0.25, 0.25))
    box("Target stand %dm" % distance, (x, y, 40), (0.3, 0.3, 0.8))
    text("%d m" % distance, (x - 10, y - 65, 275), 42)
    # Separate firing-line marks prevent near targets from occluding longer lanes.
    box("Firing mark %dm" % distance, (0, y, 1), (0.6, 1.4, 0.02))
    text("LANE %dm" % distance, (80, y - 70, 40), 25)
box("Low cover", (-300, 950, 55), (1.4, 3.0, 1.1))
box("Tall cover", (-300, 1300, 120), (1.4, 2.0, 2.4))
box("Thin geometry panel (default surface)", (1400, 1320, 130), (0.05, 2.0, 2.6))
text("GEOMETRY PANEL\nDefault physical surface; not a calibrated material test", (1350, 1150, 300), 22)
text("READY OR NOT / NATIVE GUN LAB", (-750, -1200, 360), 65)
text("F5/F6: PREVIOUS/NEXT GUN\nF7: MANUAL AMMO REFILL\nF8: RETURN TO LINE\nF9: EQUIP AUDIT / F10: OVERLAY\nNative project fire, ADS, reload and movement bindings apply.", (-700, -1200, 265), 34)
text("Baseline: native Blueprint defaults + 4 magazines.\nNo custom recoil, spread, projectile, camera or reload implementation.\nInspect native asset warnings before judging audiovisual parity.", (-650, 450, 280), 26)

start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120))
start.set_actor_label("GunLab Native Player Start")
start.set_editor_property("player_start_tag", "Blue")
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 700), unreal.Rotator(pitch=-45, yaw=-35, roll=0))
sun.light_component.set_editor_property("intensity", 6.0)
sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
fill = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 600), unreal.Rotator(pitch=-65, yaw=145, roll=0))
fill.light_component.set_editor_property("intensity", 2.0)
fill.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky = actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))

mode = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Blueprints/Games/GM_FreeMode")
assert mode, "Native FreeMode class failed to load"
world.get_world_settings().set_editor_property("default_game_mode", mode)
lab = actors.spawn_actor_from_class(unreal.RonGunLab, unreal.Vector(0, 0, 0))
lab.set_actor_label("Native Gun Lab Controller")
def selection_order(entry):
    path = entry["asset_path"]
    group = 0 if "/WeaponsRevised/Primary_" in path else 1 if "/WeaponsRevised/Secondary_" in path else 2 if "/WeaponsRevised/" in path else 3 if "/WeaponsSuspect/" in path else 4
    return group, path
paths = [entry["class_path"] for entry in sorted(catalog["weapons"], key=selection_order)]
lab.set_editor_property("weapon_classes", [unreal.load_class(None, path) for path in paths])
lab.set_editor_property("supplied_magazines", 4)
assert levels.save_current_level(), "Failed saving generated map"
receipt = {"schema": 1, "map": MAP, "actor_count": len(actors.get_all_level_actors()),
           "native_game_mode": mode.get_path_name(), "weapon_class_count": len(paths),
           "target_distances_m": [5, 10, 25, 50, 100], "generated": True,
           "playtest": "Not implied by successful map generation"}
with open(os.path.join(ROOT, "generation_receipt.json"), "w", encoding="utf-8") as handle:
    json.dump(receipt, handle, indent=2)
unreal.log("GUNLAB_MAP_SAVED " + json.dumps(receipt))
