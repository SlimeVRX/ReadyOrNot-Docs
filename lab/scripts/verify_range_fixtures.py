"""Read-only placed-actor geometry, collision and duplicate audit for GunLab.

This is editor inspection, not evidence of gameplay damage or glass fracture.
The native ExperienceProbe must perform those checks in the running game.
"""
import collections
import json
import os
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level("/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab")
world = unreal.EditorLevelLibrary.get_editor_world()
actors = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
channel_names = [n for n in dir(unreal.CollisionChannel) if n.isupper()]
projectile_channel = next((getattr(unreal.CollisionChannel, n) for n in ("ECC_PROJECTILE", "PROJECTILE", "GAME_TRACE_CHANNEL1", "GAME_TRACE_CHANNEL_1") if hasattr(unreal.CollisionChannel, n)), None)
if projectile_channel is None:
    try:
        # ReadyOrNot.h maps ECC_PROJECTILE to ECC_GameTraceChannel1 = 14.
        projectile_channel = unreal.CollisionChannel.cast(14)
    except Exception:
        pass

def vec(v):
    return [v.x, v.y, v.z]

def inspect(a):
    origin, extent = a.get_actor_bounds(False)
    row = {"label": a.get_actor_label(), "class": a.get_class().get_path_name(),
           "origin": vec(origin), "extent": vec(extent), "components": []}
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        item = {"name": c.get_name(), "class": c.get_class().get_name()}
        for name in ("get_collision_enabled", "get_collision_profile_name", "get_collision_object_type"):
            try:
                item[name] = str(getattr(c, name)())
            except Exception as exc:
                item[name] = str(exc)
        try:
            item["projectile_response"] = str(c.get_collision_response_to_channel(projectile_channel)) if projectile_channel is not None else "Custom channel is hidden from Python enum"
        except Exception as exc:
            item["projectile_response_error"] = str(exc)
        try:
            center, size, radius = unreal.SystemLibrary.get_component_bounds(c)
            item["origin"], item["extent"] = vec(center), vec(size)
        except Exception as exc:
            item["bounds_error"] = str(exc)
        if isinstance(c, unreal.StaticMeshComponent):
            item["mesh"] = c.static_mesh.get_path_name() if c.static_mesh else None
        row["components"].append(item)
    if "GunLabProceduralGlass" in [str(t) for t in a.tags]:
        row["blueprint_properties"] = {}
        for prop in ("InputMesh", "Cuts", "Position Jitter", "Rotation Jitter", "DestroyTimeLimit"):
            try:
                value = a.get_editor_property(prop)
                row["blueprint_properties"][prop] = value.get_path_name() if isinstance(value, unreal.Object) else str(value)
            except Exception as exc:
                row["blueprint_properties"][prop] = str(exc)
        # Check both directions along each axis; original BeginPlay later swaps
        # the authored static surface to procedural geometry.
        row["editor_complex_trace"] = []
        for delta in (unreal.Vector(500, 0, 0), unreal.Vector(0, 500, 0)):
            try:
                result = unreal.SystemLibrary.line_trace_single_by_profile(
                    world, origin + delta, origin - delta, "BlockAll", True, [],
                    unreal.DrawDebugTrace.NONE, True)
                if result:
                    hit = unreal.GameplayStatics.break_hit_result(result)
                    row["editor_complex_trace"].append({"blocking_hit": hit[0], "distance_cm": hit[3],
                        "impact_point": vec(hit[5]), "actor": hit[9].get_path_name() if hit[9] else None,
                        "component": hit[10].get_name() if hit[10] else None})
                else:
                    row["editor_complex_trace"].append({"blocking_hit": False})
            except Exception as exc:
                row["editor_complex_trace"].append(str(exc))
    return row

owned = [a for a in actors if "GunLabV2" in [str(t) for t in a.tags]]
labels = collections.Counter(a.get_actor_label() for a in owned)
tags = collections.Counter(str(t) for a in actors for t in a.tags)
interesting = ("GunLabProceduralGlass", "GunLabTrainingTarget", "GunLabBreachDoor", "GunLabLoadoutPortal")
report = {"actor_count": len(actors), "owned_actor_count": len(owned),
          "python_collision_channel_names": channel_names,
          "duplicate_owned_labels": {k: v for k, v in labels.items() if v > 1},
          "tag_counts": dict(tags),
          "fixtures": [inspect(a) for a in actors if any(t in [str(v) for v in a.tags] for t in interesting)]}
out = os.path.join(unreal.Paths.project_saved_dir(), "GunLab", "range_fixture_geometry.json")
with open(out, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)
assert not report["duplicate_owned_labels"], report["duplicate_owned_labels"]
assert tags["GunLabDamageSpawner"] == 3
assert tags["GunLabProceduralGlass"] == 1
assert tags["GunLabTrainingTarget"] == 2
unreal.log("GUNLAB_FIXTURE_GEOMETRY_DONE " + out)
