"""Creates /Game/VRT/Maps/L_Test: floor, hand-made 6x6 labyrinth (2 m walls), lights, fog, PlayerStart.

Run inside the UE 4.27 editor (Python Editor Script Plugin enabled):
    Output Log -> Cmd dropdown -> Python:
    py "E:/UE_projects/VRT/Tools/Editor/create_test_level.py"
or File > Execute Python Script...

Units are centimetres. Edit MAZE to change the layout, then run again (the map is recreated).
"""
import unreal

MAP_PATH = "/Game/VRT/Maps/L_Test"
CELL = 350.0          # corridor width between wall centres, cm (3.5 m)
WALL_HEIGHT = 200.0   # 2 m
WALL_THICKNESS = 20.0

# Cell = 3 chars wide, 2 lines tall. '+' corner, '-' horizontal wall, '|' vertical wall.
# The spawn is the bottom-left cell. Every cell is reachable (checked when the script was written).
MAZE = """\
+--+--+--+--+--+--+
|     |           |
+  +  +  +--+--+  +
|  |  |  |        |
+  +  +  +  +--+--+
|  |     |        |
+  +--+--+--+--+  +
|              |  |
+--+--+--+--+  +  +
|              |  |
+  +--+--+--+--+  +
|                 |
+--+--+--+--+--+--+""".split("\n")

CUBE = "/Engine/BasicShapes/Cube.Cube"
WALL_MATERIAL = "/Game/StarterContent/Materials/M_Brick_Clay_New"
FLOOR_MATERIAL = "/Game/StarterContent/Materials/M_Ground_Gravel"
SPAWN_MATERIAL = "/Game/StarterContent/Materials/M_Metal_Gold"


def load(path):
    try:
        return unreal.EditorAssetLibrary.load_asset(path)
    except Exception:
        return None


def spawn_box(label, center, size, mesh, material, folder):
    """Spawn a static mesh cube. size = full extents in cm; the engine cube is 100 cm wide."""
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center))
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    if material:
        comp.set_material(0, material)
    return actor


def main():
    rows = (len(MAZE) - 1) // 2
    cols = (len(MAZE[0]) - 1) // 3
    assert all(len(line) == 3 * cols + 1 for line in MAZE), "MAZE lines must all have the same width"

    unreal.EditorAssetLibrary.make_directory("/Game/VRT/Maps")
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.EditorAssetLibrary.delete_asset(MAP_PATH)
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        unreal.log_error("Could not create level " + MAP_PATH)
        return

    cube = load(CUBE)
    wall_mat, floor_mat, spawn_mat = load(WALL_MATERIAL), load(FLOOR_MATERIAL), load(SPAWN_MATERIAL)

    width, depth = cols * CELL, rows * CELL

    # Floor: top surface at Z = 0, 10 m margin around the maze.
    margin = 1000.0
    spawn_box("Floor", (width / 2, depth / 2, -10.0), (width + 2 * margin, depth + 2 * margin, 20.0), cube, floor_mat, "Level")

    # Walls: one box per wall segment between grid vertices.
    z = WALL_HEIGHT / 2
    count = 0
    for r in range(rows + 1):
        for c in range(cols):
            if MAZE[2 * r][3 * c + 1] == "-":
                spawn_box("Wall_H_%d_%d" % (r, c), (c * CELL + CELL / 2, r * CELL, z),
                          (CELL + WALL_THICKNESS, WALL_THICKNESS, WALL_HEIGHT), cube, wall_mat, "Maze")
                count += 1
    for r in range(rows):
        for c in range(cols + 1):
            if MAZE[2 * r + 1][3 * c] == "|":
                spawn_box("Wall_V_%d_%d" % (r, c), (c * CELL, r * CELL + CELL / 2, z),
                          (WALL_THICKNESS, CELL + WALL_THICKNESS, WALL_HEIGHT), cube, wall_mat, "Maze")
                count += 1

    # Spawn: bottom-left cell, facing +X (the open corridor). A flat gold pad marks it.
    sx, sy = CELL / 2, (rows - 1) * CELL + CELL / 2
    start = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(sx, sy, 92.0), unreal.Rotator(0, 0, 0))
    start.set_actor_label("PlayerStart")
    spawn_box("SpawnMarker", (sx, sy, 1.0), (120.0, 120.0, 2.0), cube, spawn_mat, "Level")

    # Lighting: all movable, nothing baked.
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
    unreal.log("L_Test created: %dx%d cells, %d walls, spawn at (%.0f, %.0f)" % (cols, rows, count, sx, sy))


main()
