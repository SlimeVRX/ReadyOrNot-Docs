"""One-time visual correction for the original generated lab, never game assets."""
import datetime
import hashlib
import json
import os
import shutil
import unreal

MAP = "/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab"
saved = os.path.join(unreal.Paths.project_saved_dir(), "GunLab")
disk_map = os.path.join(unreal.Paths.project_content_dir(), "ReadyOrNot/Level/Study/ReadyOrNot_GunLab.umap")
assert os.path.isfile(disk_map), "Only an existing generated lab may be repaired"
backup = os.path.join(saved, "ReadyOrNot_GunLab-before-label-rotation-fix.umap")
assert not os.path.exists(backup), "Repair already attempted; inspect the existing backup first"
shutil.copy2(disk_map, backup)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(MAP)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
assert sum(isinstance(actor, unreal.RonGunLab) for actor in actors) == 1
fixed = 0
for actor in actors:
    if isinstance(actor, unreal.TextRenderActor):
        location = actor.get_actor_location()
        actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=0 if location.x < 0 else 180, roll=0), False)
        actor.get_component_by_class(unreal.TextRenderComponent).set_text_render_color(unreal.Color(r=25, g=180, b=220, a=255))
        fixed += 1
    elif isinstance(actor, unreal.DirectionalLight):
        is_sun = actor.get_actor_location().z > 650
        actor.set_actor_rotation(unreal.Rotator(pitch=-45 if is_sun else -65, yaw=-35 if is_sun else 145, roll=0), False)
assert levels.save_current_level()
receipt_path = os.path.join(saved, "generation_receipt.json")
with open(receipt_path, encoding="utf-8") as handle:
    receipt = json.load(handle)
receipt["visual_correction_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
receipt["text_actors_corrected"] = fixed
receipt["visual_correction"] = "Named Python Rotator and Color arguments; labels face firing line, original light pitch/yaw intent restored"
with open(disk_map, "rb") as handle:
    receipt["map_sha256"] = hashlib.sha256(handle.read()).hexdigest().upper()
with open(receipt_path, "w", encoding="utf-8") as handle:
    json.dump(receipt, handle, indent=2)
unreal.log("GUNLAB_LABELS_REPAIRED " + str(fixed))
