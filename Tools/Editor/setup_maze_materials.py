"""Makes /Game/VRT/Materials/M_MazeWall: a copy of the StarterContent brick material with "Used with Instanced Static
Meshes" switched on. The maze walls are instanced meshes (AVRTMazeBuilder), and a material without that flag falls back
to the default grey material in a packaged game ("missing bUsedWithInstancedStaticMeshes=True" in the log).

Run once inside the UE 4.27 editor (File > Execute Python Script...). It is safe to run again.
"""
import unreal

SOURCE = "/Game/StarterContent/Materials/M_Brick_Clay_New"
TARGET_DIR = "/Game/VRT/Materials"
TARGET_NAME = "M_MazeWall"
TARGET = TARGET_DIR + "/" + TARGET_NAME


def main():
    unreal.EditorAssetLibrary.make_directory(TARGET_DIR)
    if not unreal.EditorAssetLibrary.does_asset_exist(TARGET):
        if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE):
            unreal.log_error("Source material " + SOURCE + " not found")
            return
        if unreal.EditorAssetLibrary.duplicate_asset(SOURCE, TARGET) is None:
            unreal.log_error("Could not duplicate " + SOURCE)
            return

    material = unreal.EditorAssetLibrary.load_asset(TARGET)
    if not isinstance(material, unreal.Material):
        unreal.log_error(TARGET + " is not a base Material, so the flag cannot be set on it")
        return

    material.set_editor_property("used_with_instanced_static_meshes", True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(TARGET)
    unreal.log(TARGET + " ready: Used with Instanced Static Meshes is on")


main()
