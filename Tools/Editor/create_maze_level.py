"""Creates /Game/VRT/Maps/L_Maze3 (see MAP_NAME): the procedural maze map (plan step 10).

The map only holds lights, fog, one AVRTMazeBuilder and a NavMeshBoundsVolume (the builder resizes it to each maze). The builder generates the maze and builds the walls, the
floor and the PlayerStart when the game starts, so nothing about the maze itself is baked into the map.
Change the maze in the editor: select MazeBuilder and edit its "VRT|Maze" properties (size, seed, braiding, rooms).

Run inside the UE 4.27 editor (Python Editor Script Plugin and Editor Scripting Utilities enabled):
    File > Execute Python Script... > this file
"""
import unreal

# Naming rule (same as create_test_level.py): bump the number whenever this script changes, keep old maps.
MAP_NAME = "L_Maze3"
MAP_PATH = "/Game/VRT/Maps/" + MAP_NAME


def main():
    unreal.EditorAssetLibrary.make_directory("/Game/VRT/Maps")
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        # The map may be open in the editor, so it can't be deleted: open it and clear it out instead.
        if not unreal.EditorLevelLibrary.load_level(MAP_PATH):
            unreal.log_error("Could not open level " + MAP_PATH)
            return
        for actor in unreal.EditorLevelLibrary.get_all_level_actors():
            # Keep the world settings and the level's default builder brush. A NavMeshBoundsVolume is also a Brush
            # subclass, so compare the exact class name, otherwise old volumes pile up with every run.
            if actor.get_class().get_name() in ("WorldSettings", "Brush"):
                continue
            unreal.EditorLevelLibrary.destroy_actor(actor)
    elif not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        unreal.log_error("Could not create level " + MAP_PATH)
        return

    builder = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.VRTMazeBuilder, unreal.Vector(0, 0, 0))
    builder.set_actor_label("MazeBuilder")
    builder.set_folder_path("Maze")

    # Navigation: the builder fits this volume to every maze it builds. Any size works as a starting point.
    try:
        nav = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(1600, 1600, 250))
        nav.set_actor_label("NavMeshBounds")
        nav.set_folder_path("Maze")
        nav.set_actor_scale3d(unreal.Vector(20, 20, 5))
    except Exception as exc:
        unreal.log_warning("Could not place a NavMeshBoundsVolume (%s). Place one by hand: Place Actors > Volumes > Nav Mesh Bounds Volume." % exc)

    # Lighting: all movable, nothing baked (the maze is different every run).
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(0, -50, 30))
    sun.set_actor_label("Sun")
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)

    sky_light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky_light.set_actor_label("SkyLight")
    sky_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
    sky_comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_comp.set_editor_property("real_time_capture", True)

    unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)).set_actor_label("SkyAtmosphere")
    unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 100)).set_actor_label("Fog")

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log(MAP_NAME + " created: one MazeBuilder, a NavMeshBoundsVolume, sun, sky light, atmosphere and fog")


main()
