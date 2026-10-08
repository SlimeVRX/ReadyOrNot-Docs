"""Upgrade only the generated local GunLab map with existing native game systems.

Requires the compiled lab plugin. Backs up the map before changes; reruns replace
only actors tagged GunLabV2 and the original generator's explicitly named cubes.
No original ReadyOrNot assets or player profiles are edited.
"""
import datetime
import hashlib
import json
import os
import shutil
import unreal

MAP = "/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab"
OUT = os.path.join(unreal.Paths.project_saved_dir(), "GunLab")
DISK = os.path.join(unreal.Paths.project_content_dir(), "ReadyOrNot/Level/Study/ReadyOrNot_GunLab.umap")
TAG = "GunLabV2"
os.makedirs(OUT, exist_ok=True)
assert os.path.isfile(DISK), "Create the original generated lab first"
assert all(hasattr(unreal, n) for n in ("RonGunLabFixtures", "RonGunLabExperienceProbe", "RonGunLabGuide")), "Build latest lab plugin first"
stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d-%H%M%S")
backup = os.path.join(OUT, "ReadyOrNot_GunLab-before-native-upgrade-" + stamp + ".umap")
assert not os.path.exists(backup)
shutil.copy2(DISK, backup)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(MAP)
world = unreal.EditorLevelLibrary.get_editor_world()
assert len([a for a in actors.get_all_level_actors() if isinstance(a, unreal.RonGunLab)]) == 1
for a in actors.get_all_level_actors():
    if isinstance(a, unreal.RonGunLab):
        a.set_editor_property("show_overlay", False)
report = {"schema": 2, "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "map": MAP,
          "backup": backup, "fixtures": [], "materials": [], "doors": [], "volumes": [], "warnings": []}

def table_rows(path, columns):
    table = unreal.load_asset(path)
    assert table, path
    names = [str(n) for n in unreal.DataTableFunctionLibrary.get_data_table_row_names(table)]
    values = {column: list(unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, column)) for column in columns}
    return table, [{"row": name, **{key: (str(vals[i]) if i < len(vals) else "") for key, vals in values.items()}} for i, name in enumerate(names)]

# Select a real native AI data row with no randomly selected armour or helmet.
# Read-only column inspection keeps us from inventing a replacement AI definition.
ai_table, ai_rows = table_rows("/Game/Blueprints/DataTables/AI/AIDataTable_Training", ["CharacterClass", "SpawningTeamType", "AIBodyArmourSelection", "AIHelmetSelection", "AIBodyArmourOverride"])
report["inspected_ai_rows"] = ai_rows
suspect_rows = [r for r in ai_rows if "Suspect" in r["CharacterClass"] and r["AIBodyArmourSelection"] in ("", "()", "(None)") and r["AIHelmetSelection"] in ("", "()", "(None)") and r["AIBodyArmourOverride"] in ("", "None")]
if not suspect_rows:
    ai_table, ai_rows = table_rows("/Game/Blueprints/DataTables/AIDataTable_Test", ["CharacterClass", "SpawningTeamType", "AIBodyArmourSelection", "AIHelmetSelection", "AIBodyArmourOverride"])
    report["inspected_test_ai_rows"] = ai_rows
    suspect_rows = [r for r in ai_rows if "Suspect" in r["CharacterClass"] and r["AIBodyArmourSelection"] in ("", "()", "(None)") and r["AIHelmetSelection"] in ("", "()", "(None)") and r["AIBodyArmourOverride"] in ("", "None")]
assert suspect_rows, "No native unarmoured suspect data row found; inspect native_upgrade_receipt.json"
ai_row = suspect_rows[0]["row"]
armour_table, armour_rows = table_rows("/Game/Blueprints/DataTables/SuspectArmourDataTable", ["bIsHelmet", "ArmourLevel", "ArmourMaterial"])
report["inspected_armour_rows"] = armour_rows
body_rows = [r["row"] for r in armour_rows if r["bIsHelmet"].lower() in ("false", "0")]
assert body_rows, "Native body armour rows missing"
assert "Armour_4" in body_rows, "Expected original Armour_4 row missing"
preferred_armour = "Armour_4"
door_table = unreal.load_asset("/Game/Blueprints/DataTables/DoorDataTable")
assert door_table
archetype = unreal.load_asset("/Game/Blueprints/AI/Archetypes/Debug/Debug_Suspect_Static")
assert archetype
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
paper_mesh = unreal.load_asset("/Game/ReadyOrNot/Level/RoN_Station/Station_Killhouse/Mesh/Killhouse_Target_Cardboard_SM")
assert cube and paper_mesh
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    original_targets = [prefix + "%dm" % d for d in (5, 10, 25, 50, 100) for prefix in ("Target ", "Target center ", "Target stand ")]
    if TAG in [str(t) for t in actor.tags] or label in original_targets or label == "Thin geometry panel (default surface)" or label.startswith("GEOMETRY PANEL\n") or label.startswith("Baseline: native Blueprint defaults"):
        actors.destroy_actor(actor)

def spawn(cls, label, location, yaw=0, extra_tags=()):
    a = actors.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    assert a, label
    a.set_actor_label("GunLabV2 / " + label)
    a.set_editor_property("tags", [TAG, *extra_tags])
    return a

def box(label, location, scale, material=None):
    a = spawn(unreal.StaticMeshActor, label, location)
    a.static_mesh_component.set_static_mesh(cube)
    a.static_mesh_component.set_collision_profile_name("BlockAll")
    a.set_actor_scale3d(unreal.Vector(*scale))
    if material:
        a.static_mesh_component.set_material(0, material)
    return a

def text(label, location, size=27, yaw=0):
    a = spawn(unreal.TextRenderActor, "Sign " + label.split("\n")[0], location, yaw)
    c = a.get_component_by_class(unreal.TextRenderComponent)
    c.set_text(label)
    c.set_world_size(size)
    c.set_text_render_color(unreal.Color(r=25, g=180, b=220, a=255))
    return a

def volume(cls, label, location, size):
    a = spawn(cls, label, location)
    assert unreal.RonGunLabFixtures.build_editor_box_volume(a, unreal.Vector(*size)), label
    report["volumes"].append({"label": label, "class": cls.static_class().get_path_name() if hasattr(cls, "static_class") else str(cls), "location": location, "full_size_cm": size})
    return a

def pm_material(name):
    pm = unreal.load_asset("/Game/ReadyOrNot/Data/Phys-Materials/RON_" + name + "_PM")
    assert pm, name
    path = "/Game/ReadyOrNot/Level/Study/GunLabMaterials/MI_GunLab_" + name
    mat = unreal.load_asset(path)
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("MI_GunLab_" + name, "/Game/ReadyOrNot/Level/Study/GunLabMaterials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    assert mat
    unreal.MaterialEditingLibrary.set_material_instance_parent(mat, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    mat.set_editor_property("phys_material", pm)
    unreal.MaterialEditingLibrary.update_material_instance(mat)
    assert unreal.EditorAssetLibrary.save_asset(path)
    return pm, mat

pm_concrete, concrete = pm_material("Concrete_Strong")
box("Native experience annex floor", (-2850, 0, -25), (57, 56, 0.5), concrete)
box("Annex backstop", (-5650, 0, 220), (0.3, 56, 4.4), concrete)
box("Annex left side", (-2850, -2800, 220), (57, 0.3, 4.4), concrete)
box("Annex right side", (-2850, 2800, 220), (57, 0.3, 4.4), concrete)

# Preserve exact authored paper target mesh / material for native bullet impacts.
for i, distance in enumerate((5, 10, 25, 50, 100)):
    if distance in (5, 10):
        bp = "/Game/ReadyOrNot/Shoothouse/BP_TrainingTarget" + ("Hostage" if distance == 10 else "")
        target_cls = unreal.EditorAssetLibrary.load_blueprint_class(bp)
        assert target_cls, bp
        a = spawn(target_cls, "Native hit-area target %dm" % distance, (distance * 100, (i - 2) * 550, 0), 270,
                  ("GunLabTrainingTarget",))
    else:
        a = spawn(unreal.StaticMeshActor, "Native paper target %dm" % distance, (distance * 100, (i - 2) * 550, 0), 270)
        a.static_mesh_component.set_static_mesh(paper_mesh)
        a.static_mesh_component.set_collision_profile_name("BlockAll")
text("NATIVE GUNPLAY ANNEX  <  behind firing line\nLoadout / ammunition / armour / damage / material impacts / breaching", (-50, -1300, 390), 27)

box("Loadout station desk", (-450, -620, 48), (1.2, 1.8, 0.96), concrete)
portal = spawn(unreal.LoadoutPortal, "Native loadout portal", (-380, -620, 140), extra_tags=("GunLabLoadoutPortal",))
text("NATIVE LOADOUT [Use]\nWeapons / attachments / ammo / gear\nNative UI saves your chosen preset on exit", (-370, -750, 210), 20)
ammo_cls = unreal.EditorAssetLibrary.load_blueprint_class("/Game/ReadyOrNot/Level/Dev/Level_Design/Blueprints/BP_AmmoRefillBox_01")
assert ammo_cls
spawn(ammo_cls, "Native ammo refill box", (-420, -1100, 0), extra_tags=("GunLabAmmoStation",))
text("NATIVE REFILL [Use]\nReapplies your current loadout", (-300, -1210, 190), 22)

# Native AI spawners supply real mesh, health zones, armour, hit reaction, stun,
# morale and ragdoll through AAISpawn::DoSpawn -> FinishAISpawning.
for index, (y, armour, label) in enumerate(((-1600, "None", "UNARMOURED"), (-850, preferred_armour, "ARMOURED"), (-100, "None", "LESS LETHAL / STUN"))):
    a = spawn(unreal.AISpawn, "Native damage target " + label, (-4700, y, 100), 0,
              ("GunLabDamageSpawner", "GunLabTarget%d" % index))
    data = unreal.SpawnData()
    handle = unreal.DataTableRowHandle()
    handle.set_editor_property("data_table", ai_table)
    handle.set_editor_property("row_name", ai_row)
    data.set_editor_property("spawned_ai", handle)
    data.set_editor_property("force_no_weapon", True)
    data.set_editor_property("deactivated", False)
    data.set_editor_property("force_body_armour_override", armour)
    data.set_editor_property("spawn_with_tags", ["GunLabDamageTarget", "GunLabTarget%d" % index])
    a.set_editor_property("spawn_array", [data])
    a.set_editor_property("enabled", True)
    a.set_editor_property("allow_explosive_vest_spawn", False)
    a.set_editor_property("archetype_override", archetype)
    box("Target bay divider %d" % index, (-4560, y - 300, 115), (4, 0.18, 2.3), concrete)
    text(label + "\nNative AI: body/head damage, reactions and ragdoll\nronlab_targets reset", (-4350, y - 170, 310), 22)
    report["fixtures"].append({"label": label, "spawner": a.get_name(), "data_table": ai_table.get_path_name(), "row": ai_row, "armour_override": armour, "archetype": archetype.get_path_name(), "force_no_weapon": True, "deactivated": False, "location_cm": [-4700, y, 100]})

# Side-by-side material/thickness fixtures use original physical materials and
# a lab-owned material instance so complex traces resolve the native surface.
for index, (name, thickness) in enumerate((("Drywall", 2), ("Plywood", 5), ("Steel", 2), ("Concrete_Strong", 20), ("Glass_Plate", 2), ("Aluminium", 2))):
    y = -2100 + index * 500
    pm, material = pm_material(name)
    panel = box("Material " + name, (-2100, y, 150), (thickness / 100.0, 2.5, 2.6), material)
    panel.static_mesh_component.set_phys_material_override(pm)
    panel.set_editor_property("tags", [TAG, "GunLabMaterialPanel", "GunLabMaterial" + name])
    catcher = spawn(unreal.StaticMeshActor, "Impact witness " + name, (-2450, y, 0), 90)
    catcher.static_mesh_component.set_static_mesh(paper_mesh)
    catcher.static_mesh_component.set_collision_profile_name("BlockAll")
    text("%s / %d cm\nNative surface + witness target\nAmmo and angle determine the result" % (name, thickness), (-2000, y - 145, 325), 20)
    report["materials"].append({"name": name, "thickness_cm": thickness, "physical_material": pm.get_path_name(), "surface_type": str(pm.surface_type), "material_instance": material.get_path_name()})

# Original Blueprint fracture logic, separate from the fixed-thickness PM panel.
# Never substitute a lookalike plane and call it destructible glass.
glass_cls = None
for glass_path in ("/Game/ReadyOrNot/Level/RoN_Valley/Valley_Arch/BP_BreakableGlass", "/Game/ThirdParty/ProceduralGlass/Blueprints/BP_ProceduralDestructibleGlass"):
    candidate = unreal.EditorAssetLibrary.load_blueprint_class(glass_path)
    if candidate and isinstance(unreal.get_default_object(candidate), unreal.BreakableGlass):
        glass_cls = candidate
        break
assert glass_cls, "No original ABreakableGlass Blueprint found"
glass = spawn(glass_cls, "Native breakable glass", (-3100, 1100, 135), 90, ("GunLabBreakableGlass",))
text("NATIVE BREAKABLE GLASS\nOriginal projectile-hit / Blueprint surface logic\nLegacy Apex fragments unavailable in this snapshot\nReload map to restore", (-2700, 850, 320), 22)
report["breakable_glass"] = {"class": glass_cls.get_path_name(), "location_cm": [-3100, 1100, 135], "native_parent": "/Script/ReadyOrNot.BreakableGlass"}
glass_origin, glass_extent = glass.get_actor_bounds(False)
report["breakable_glass"]["bounds_extent_cm"] = [glass_extent.x, glass_extent.y, glass_extent.z]
procedural_source = "/Game/ThirdParty/ProceduralGlass/Blueprints/BP_ProceduralDestructibleGlass"
procedural_copy = "/Game/ReadyOrNot/Level/Study/GlassRecovery/BP_GunLab_ProceduralGlass"
source_disk = os.path.join(unreal.Paths.project_content_dir(), "ThirdParty/ProceduralGlass/Blueprints/BP_ProceduralDestructibleGlass.uasset")
with open(source_disk, "rb") as handle:
    original_glass_hash = hashlib.sha256(handle.read()).hexdigest().upper()
duplicate = unreal.load_asset(procedural_copy)
if not duplicate:
    source_blueprint = unreal.load_asset(procedural_source)
    assert source_blueprint
    duplicate = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        "BP_GunLab_ProceduralGlass", "/Game/ReadyOrNot/Level/Study/GlassRecovery", source_blueprint)
assert duplicate
glass_restore = json.loads(unreal.RonGunLabFixtures.restore_original_glass_events(duplicate))
glass_restore.update({"source_asset": procedural_source, "local_copy": procedural_copy,
                      "source_sha256": original_glass_hash,
                      "scope": "Restore three disconnected original event entry wires only; original slicing, forces, timers and sound remain unchanged"})
with open(os.path.join(OUT, "procedural_glass_restore_receipt.json"), "w", encoding="utf-8") as handle:
    json.dump(glass_restore, handle, indent=2)
assert glass_restore["success"], glass_restore
assert unreal.EditorAssetLibrary.save_asset(procedural_copy)
with open(source_disk, "rb") as handle:
    assert hashlib.sha256(handle.read()).hexdigest().upper() == original_glass_hash, "Original glass asset must remain unchanged"
report["procedural_glass_restore"] = glass_restore
procedural_cls = unreal.EditorAssetLibrary.load_blueprint_class(procedural_copy)
assert procedural_cls
procedural_glass = spawn(procedural_cls, "Original procedural glass", (-3150, 400, 135), 90, ("GunLabProceduralGlass",))
text("ORIGINAL PROCEDURAL GLASS\nOriginal event entries restored in lab-only copy\nNative point-hit -> original Blueprint slicing / physics\nReload map to restore", (-2800, 100, 330), 21)
report["procedural_glass"] = {"class": procedural_cls.get_path_name(), "location_cm": [-3150, 400, 135], "point_damage_adapter": "OnTakePointDamage -> original OnGlassHit(HitResult, DamageCauser)", "physics_and_fracture": "Existing project Blueprint slicing; three original event entries restored in a lab-only copy", "validation": "Requires runtime fragmentation observation"}

# A bounded room supplies close obstruction, weapon raising, lean and flashlight
# tests; native doors preserve the original interaction and destruction logic.
box("CQB room back", (-4750, 2400, 150), (15, 0.2, 3), concrete)
box("CQB room left", (-5500, 1600, 150), (0.2, 16, 3), concrete)
box("CQB room right", (-4000, 1600, 150), (0.2, 16, 3), concrete)
box("CQB roof", (-4750, 1600, 310), (15, 16, 0.2), concrete)
box("CQB entrance left", (-5070, 800, 150), (8.6, 0.2, 3), concrete)
box("CQB entrance right", (-4245, 800, 150), (4.9, 0.2, 3), concrete)
box("CQB entrance lintel", (-4565, 800, 275), (1.5, 0.2, 0.5), concrete)
door_cls = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Blueprints/Environment/BP_Door_New")
assert door_cls
native_doors = []
for row, loc, yaw in (("Default_Wood", (-4500, 800, 0), 90), ("Hotel_Steel_01", (-3300, 2000, 0), 0)):
    a = spawn(door_cls, "Native door " + row, loc, yaw, ("GunLabBreachDoor",))
    handle = unreal.DataTableRowHandle()
    handle.set_editor_property("data_table", door_table)
    handle.set_editor_property("row_name", row)
    a.set_editor_property("type_of_door", handle)
    a.set_editor_property("randomly_open_at_game_start", False)
    a.set_editor_property("override_lock_chance", True)
    a.set_editor_property("locked_chance", 1.0)
    native_doors.append(a)
    report["doors"].append({"row": row, "class": door_cls.get_path_name(), "location": loc, "lock_chance": 1.0})
text("NATIVE BREACH / CQB ROOM\nUse / peek / kick / lockpick / shotgun / C2\nReload map to restore broken doors", (-4450, 470, 315), 23, 270)
for offset, height in ((0, 100), (420, 190)):
    box("CQB ready/lean cover %d" % offset, (-4800, 1600 + offset, height / 2), (1.2, 2.3, height / 100), concrete)
light = spawn(unreal.PointLight, "Dim CQB practical light", (-4750, 1600, 265))
light.point_light_component.set_editor_property("intensity", 250.0)
light.point_light_component.set_editor_property("attenuation_radius", 850.0)
light.point_light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)

# Original sound graph components; no replacement gunshot/reverb implementation.
range_room = volume(unreal.RoomVolume, "Range sound room", (5450, 0, 280), (12900, 3100, 560))
annex_room = volume(unreal.RoomVolume, "Annex sound room", (-2500, 0, 280), (3000, 5500, 560))
damage_room = volume(unreal.RoomVolume, "Damage bay sound room", (-4800, -1000, 280), (1600, 3500, 560))
cqb_room = volume(unreal.RoomVolume, "CQB sound room", (-4750, 1600, 145), (1480, 1580, 290))
for a in (range_room, annex_room, damage_room):
    a.set_editor_property("room_group_id", 1)
cqb_room.set_editor_property("room_group_id", 2)
door_portal = volume(unreal.PortalVolume, "Door sound portal", (-4560, 800, 130), (160, 110, 260))
door_portal.set_editor_property("attached_objects", [native_doors[0]])
door_portal.set_editor_property("is_outside", True)
entrance_portal = volume(unreal.PortalVolume, "Range outside sound portal", (-1000, 0, 250), (110, 3000, 500))
entrance_portal.set_editor_property("is_outside", True)
indoor_audio = volume(unreal.ReadyOrNotAudioVolume, "CQB native reverb", (-4750, 1600, 145), (1480, 1580, 290))
reverb_events = [unreal.load_asset("/Game/FMOD/Events/Levels/Reverbs/Commands/ReverbCommand_Small"), unreal.load_asset("/Game/FMOD/Events/Levels/Reverbs/Commands/Materials/ReverbMaterialCommand_Concrete")]
assert all(reverb_events)
indoor_audio.set_editor_property("reverb_events", reverb_events)
outdoor_audio = volume(unreal.ReadyOrNotAudioVolume, "Outdoor native reverb", (5200, 0, 260), (13300, 5500, 520))
street_event = unreal.load_asset("/Game/FMOD/Events/Levels/Reverbs/Commands/ReverbCommand_Street")
assert street_event
outdoor_audio.set_editor_property("reverb_events", [street_event])
volume(unreal.NavMeshBoundsVolume, "Native navigation bounds", (3100, 0, 150), (18100, 5500, 500))
spawn(unreal.RonGunLabFixtures, "Native fixture lifecycle", (0, 0, 0))
spawn(unreal.RonGunLabExperienceProbe, "Native experience QA (opt-in only)", (0, 0, 0))
spawn(unreal.RonGunLabGuide, "Native Gun Lab guide", (0, 0, 0))

text("NATIVE GUNPLAY / TEST ROUTE\n1 Loadout, attachments, ammunition\n2 Paper range: trigger modes / ADS / reload / mag check\n3 Native damage targets: armour / stun / reactions\n4 Material panels: penetration and impact response\n5 Locked doors + dark CQB room: breach / ready / light\nUse native bindings; no automatic infinite health or ammunition", (-850, 1100, 370), 24)
report["actor_count"] = len(actors.get_all_level_actors())
report["native_systems"] = ["LoadoutPortal", "AmmoRefillBox", "AISpawn", "CharacterHealthComponent", "SuspectArmour", "Door", "BreakableGlass", "TrainingTarget", "CustomPhysicalMaterial", "RoomVolume", "PortalVolume", "ReadyOrNotAudioVolume"]
report["limitations"] = ["Static unarmed debug-archetype AI fixtures isolate native damage/stun response; this is not a full combat encounter.", "Paper mesh is a visual impact witness; no replacement health or scoring component is added.", "Material geometry and thickness are lab fixtures; native ammo/surface rules determine penetration/ricochet.", "Authored FMOD event references are attached; audible mix and propagation still require runtime listening.", "Native Valley breakable-glass Blueprint references missing ApexDestruction components and missing old textures; native hit/material path is present but full fragment fracture is not claimed."]
assert levels.save_current_level(), "Saving generated lab failed"
with open(DISK, "rb") as handle:
    report["map_sha256"] = hashlib.sha256(handle.read()).hexdigest().upper()
with open(os.path.join(OUT, "native_upgrade_receipt.json"), "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2, ensure_ascii=False)
unreal.log("GUNLAB_NATIVE_UPGRADE_DONE actors=%d targets=%d materials=%d doors=%d" % (report["actor_count"], len(report["fixtures"]), len(report["materials"]), len(report["doors"])))
