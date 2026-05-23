bl_info = {
    "name": "glTF Tag UI",
    "author": "whydidyoumakethis",
    "version": (1, 0, 0),
    "blender": (5, 0, 1),
    "location": "View3D > Sidebar > Game Tags",
    "description": "UI list and tagging tools for glTF extras workflow",
}

import bpy
from bpy.types import Panel, Operator, PropertyGroup, UIList
from bpy.props import (
    EnumProperty,
    CollectionProperty,
    IntProperty,
    PointerProperty,
    FloatProperty,
    BoolProperty,
)


# ---------------------------------------------------
# Preset item
# ---------------------------------------------------

class GLTFTAG_PresetItem(PropertyGroup):


    body_type: EnumProperty(
        name="Body Type",
        items=[
            ("STATIC", "Static", ""),
            ("DYNAMIC", "Dynamic", ""),
            ("KINEMATIC", "Kinematic", ""),
        ],
        default="STATIC"
    )

    collider_type: EnumProperty(
        name="Collider",
        items=[
            ("NONE", "None", ""),
            ("BOX", "Box", ""),
            ("SPHERE", "Sphere", ""),
            ("CAPSULE", "Capsule", ""),
            ("MESH", "Mesh", ""),
            ("CONVEX_HULL", "Convex Hull", ""),
        ],
        default="BOX"
    )

    misc_type: EnumProperty(
        name="Misc Tag",
        items=[
            ("NONE", "None", ""),
            ("FLOOR", "Floor", ""),
            ("LAVA", "Lava", ""),
            ("SPEED_BOOST", "Speed Boost", ""),
            ("DOUBLE_JUMP", "Double Jump", ""),
            ("DASH", "Dash", ""),
            ("TRIGGER", "Trigger", ""),
            ("PLAYER", "Player", ""),
            ("GOAL", "Goal", ""),
            ("SPAWN", "Spawn", ""),
            ("DOOR", "Door", ""),
        ],
        default="NONE"
    )

    anim_type: EnumProperty(
        name="Anim Tag",
        items=[
            ("NONE", "None", ""),
            ("UP_DOWN", "Up Down", ""),
            ("DOWN_UP", "Down Up", ""),
            ("LEFT_RIGHT", "Left Right", ""),
            ("RIGHT_LEFT", "Right Left", ""),
            ("FORWARD_BACKWARD", "Forward Backward", ""),
            ("BACKWARD_FORWARD", "Backward Forward", ""),
            ("ROTATE_CLOCKWISE", "Rotate Clockwise", ""),
            ("ROTATE_COUNTERCLOCKWISE", "Rotate Counterclockwise", ""),
        ],
        default="NONE"
    )




# ---------------------------------------------------
# Scene properties for current UI values
# ---------------------------------------------------

class GLTFTAG_SceneProps(PropertyGroup):
    body_type: EnumProperty(
        name="Body Type",
        items=[
            ("STATIC", "Static", ""),
            ("DYNAMIC", "Dynamic", ""),
            ("KINEMATIC", "Kinematic", ""),
        ],
        default="STATIC",
    )

    collider_type: EnumProperty(
        name="Collider",
        items=[
            ("NONE", "None", ""),
            ("BOX", "Box", ""),
            ("SPHERE", "Sphere", ""),
            ("CAPSULE", "Capsule", ""),
            ("MESH", "Mesh", ""),
            ("CONVEX_HULL", "Convex Hull", ""),
        ],
        default="BOX",
    )

    misc_type: EnumProperty(
        name="Misc Tag",
        items=[
            ("NONE", "None", ""),
            ("FLOOR", "Floor", ""),
            ("LAVA", "Lava", ""),
            ("SPEED_BOOST", "Speed Boost", ""),
            ("DOUBLE_JUMP", "Double Jump", ""),
            ("DASH", "Dash", ""),
            ("TRIGGER", "Trigger", ""),
            ("PLAYER", "Player", ""),
            ("GOAL", "Goal", ""),
            ("SPAWN", "Spawn", ""),
            ("DOOR", "Door", ""),
        ],
        default="NONE",
    )

    trigger_kind: EnumProperty(
        name="Trigger Kind",
        items=[
            ("NONE", "None", ""),
            ("GOAL", "Goal", ""),
            ("TELEPORT", "Teleport", ""),
        ],
        default="NONE",
    )

    trigger_half_x: FloatProperty(name="Trigger Half X", default=1.0, min=0.0)
    trigger_half_y: FloatProperty(name="Trigger Half Y", default=1.0, min=0.0)
    trigger_half_z: FloatProperty(name="Trigger Half Z", default=0.1, min=0.0)

    teleport_loop_num: IntProperty(name="Teleport Loop Num", default=0, min=0)
    teleport_order:    IntProperty(name="Teleport Order",    default=0, min=0)

    anim_type: EnumProperty(
        name="Anim Tag",
        items=[
            ("NONE", "None", ""),
            ("UP_DOWN", "Up Down", ""),
            ("DOWN_UP", "Down Up", ""),
            ("LEFT_RIGHT", "Left Right", ""),
            ("RIGHT_LEFT", "Right Left", ""),
            ("FORWARD_BACKWARD", "Forward Backward", ""),
            ("BACKWARD_FORWARD", "Backward Forward", ""),
            ("ROTATE_CLOCKWISE", "Rotate Clockwise", ""),
            ("ROTATE_COUNTERCLOCKWISE", "Rotate Counterclockwise", ""),
        ],
        default="NONE"
    )
    anim_distance: FloatProperty(
        name="Anim Distance",
        default=5.0,
        min=0.0,
    )

    anim_speed: FloatProperty(
        name="Anim Speed",
        default=1.0,
        min=0.0,
    )

    anim_rotation_speed: FloatProperty(
        name="Rotation Speed",
        default=90.0,
    )

    door_radius: FloatProperty(
        name="Door Radius",
        default=2.0,
        min=0.0,
    )
    door_angle: FloatProperty(
        name="Door Open Angle",
        default=90.0,
    )
    door_speed: FloatProperty(
        name="Door Speed",
        default=540.0,
        min=0.0,
    )


# ---------------------------------------------------
# Operators
# ---------------------------------------------------


def apply_ui_to_object(obj, ui):
    obj["body"] = ui.body_type.lower()
    obj["collider"] = ui.collider_type.lower()
    obj["misc"] = ui.misc_type.lower()
    obj["anim"] = ui.anim_type.lower()
    obj["anim_distance"] = ui.anim_distance
    obj["anim_speed"] = ui.anim_speed
    obj["anim_rotation_speed"] = ui.anim_rotation_speed

    obj["trigger_kind"] = ui.trigger_kind.lower()
    obj["trigger_half_x"] = float(ui.trigger_half_x)
    obj["trigger_half_y"] = float(ui.trigger_half_y)
    obj["trigger_half_z"] = float(ui.trigger_half_z)
    obj["teleport_loop_num"] = int(ui.teleport_loop_num)
    obj["teleport_order"]    = int(ui.teleport_order)

    obj["door_radius"] = float(ui.door_radius)
    obj["door_angle"]  = float(ui.door_angle)
    obj["door_speed"]  = float(ui.door_speed)


class GLTFTAG_OT_apply_active(Operator):
    bl_idname = "gltf_tag.apply_active"
    bl_label = "Apply to Active"

    def execute(self, context):
        obj = context.active_object
        if obj is None:
            self.report({'WARNING'}, "No active object")
            return {'CANCELLED'}

        apply_ui_to_object(obj, context.scene.gltf_tag_ui)
        self.report({'INFO'}, f"Applied tags to {obj.name}")
        return {'FINISHED'}


class GLTFTAG_OT_apply_selected(Operator):
    bl_idname = "gltf_tag.apply_selected"
    bl_label = "Apply to Selected"

    def execute(self, context):
        selected = context.selected_objects
        if not selected:
            self.report({'WARNING'}, "No selected objects")
            return {'CANCELLED'}

        ui = context.scene.gltf_tag_ui
        for obj in selected:
            apply_ui_to_object(obj, ui)

        self.report({'INFO'}, f"Applied tags to {len(selected)} object(s)")
        return {'FINISHED'}


# ---------------------------------------------------
# Panel
# ---------------------------------------------------

class GLTFTAG_PT_main_panel(Panel):
    bl_label = "glTF Tag Tools"
    bl_idname = "GLTFTAG_PT_main_panel"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Game Tags"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        ui = getattr(scene, "gltf_tag_ui", None)


        layout.prop(ui, "body_type")
        layout.prop(ui, "collider_type")
        layout.prop(ui, "misc_type")

        if ui.misc_type == "TRIGGER":
            box = layout.box()
            box.label(text="Trigger Settings")
            box.prop(ui, "trigger_kind")
            row = box.row(align=True)
            row.prop(ui, "trigger_half_x")
            row.prop(ui, "trigger_half_y")
            row.prop(ui, "trigger_half_z")
            if ui.trigger_kind == "TELEPORT":
                box.prop(ui, "teleport_loop_num")
                box.prop(ui, "teleport_order")

        if ui.misc_type == "DOOR":
            box = layout.box()
            box.label(text="Door Settings")
            box.prop(ui, "door_radius")
            box.prop(ui, "door_angle")
            box.prop(ui, "door_speed")

        layout.separator()

        layout.prop(ui, "anim_type")
        layout.prop(ui, "anim_distance")
        layout.prop(ui, "anim_speed")
        layout.prop(ui, "anim_rotation_speed")

        layout.separator()

        layout.operator("gltf_tag.apply_active", icon='OBJECT_DATA')
        layout.operator("gltf_tag.apply_selected", icon='OUTLINER_OB_GROUP_INSTANCE')


# ---------------------------------------------------
# Registration
# ---------------------------------------------------

core_classes = (
    GLTFTAG_PresetItem,
    GLTFTAG_SceneProps,
)

ui_classes = (
    GLTFTAG_OT_apply_active,
    GLTFTAG_OT_apply_selected,
    GLTFTAG_PT_main_panel,
)

classes = core_classes + ui_classes

def register():
    # Clean old registrations
    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except Exception:
            pass

    # Remove old scene props if they exist
    if hasattr(bpy.types.Scene, "gltf_tag_ui"):
        del bpy.types.Scene.gltf_tag_ui
    if hasattr(bpy.types.Scene, "gltf_tag_presets"):
        del bpy.types.Scene.gltf_tag_presets
    if hasattr(bpy.types.Scene, "gltf_tag_presets_index"):
        del bpy.types.Scene.gltf_tag_presets_index

    # Register property/data classes first
    for cls in core_classes:
        bpy.utils.register_class(cls)

    # Attach scene properties before UI panel can draw
    bpy.types.Scene.gltf_tag_ui = PointerProperty(type=GLTFTAG_SceneProps)
    bpy.types.Scene.gltf_tag_presets = CollectionProperty(type=GLTFTAG_PresetItem)
    bpy.types.Scene.gltf_tag_presets_index = IntProperty(default=0)

    # Register operators + panel last
    for cls in ui_classes:
        bpy.utils.register_class(cls)

def unregister():
    if hasattr(bpy.types.Scene, "gltf_tag_ui"):
        del bpy.types.Scene.gltf_tag_ui
    if hasattr(bpy.types.Scene, "gltf_tag_presets"):
        del bpy.types.Scene.gltf_tag_presets
    if hasattr(bpy.types.Scene, "gltf_tag_presets_index"):
        del bpy.types.Scene.gltf_tag_presets_index

    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except Exception:
            pass


if __name__ == "__main__":
    
    register()