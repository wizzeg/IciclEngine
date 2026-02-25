#include <engine/game/components.h>
#include <engine/game/component_data.h>
#include <engine/editor/componentdata_macros.h>

REGISTER_COMPONENT(TestComponent, ("Test Category"),
	FIELD_DEFAULT("test: ", comp, test)
)

REGISTER_COMPONENT(EntityComponent, ("Default"),
	FIELD_CUSTOM("entity name: ", comp, hashed_name, 1.75f, false),
	FIELD_READONLY_CUSTOM("entity id: ", comp, entity, 2.75f, false)
)

REGISTER_COMPONENT(TransformDynamicComponent, ("Transform", "Rendering"),
	FIELD_CUSTOM("position: ", comp, position, 1.75f, false),
	FIELD_CUSTOM("scale: ", comp, scale, 1.75f, false),
	FIELD_CUSTOM("rotation: ", comp, rotation_euler_do_not_use, 1.75f, false),
	FIELD_CUSTOM("overwrite quaternion rotation: ", comp, overide_quaternion, 1.1f, false),
	FIELD_CUSTOM("update euler rotation: ", comp, get_euler_angles, 1.1f, false),
	FIELD_HIDDEN(comp, rotation_quat),
	FIELD_HIDDEN(comp, model_matrix)
	)

	REGISTER_COMPONENT(TransformDynamicStaticComponent, ("Transform", "Rendering"),
		FIELD_DEFAULT("dynamic: ", comp, dynamic)
	)

REGISTER_COMPONENT(TransformStaticComponent, ("Transform", "Rendering"),
	FIELD_HIDDEN(comp, model_matrix)
	)


REGISTER_COMPONENT(MeshComponent, ("Rendering"),
	FIELD_DEFAULT("mesh loaded: ", comp, loaded),
	FIELD_CUSTOM("mesh path: ", comp, hashed_path, 1.75f, false)
)

REGISTER_COMPONENT(TextureComponent, ("Rendering"),
	FIELD_DEFAULT("texture loaded: ", comp, loaded),
	FIELD_CUSTOM("texture path: ", comp, hashed_path, 1.75f, false)
)


REGISTER_COMPONENT(MaterialComponent, ("Rendering"),
	FIELD_CUSTOM("material path: ", comp, hashed_path, 1.75f, false),
	FIELD_CUSTOM("instanced: ", comp, instance, 0.75f, true),
	FIELD_CUSTOM("use mipmaps: ", comp, mipmap, 0.0f, true),
	FIELD_DEFAULT("load material: ", comp, load)
)

REGISTER_COMPONENT(CameraComponent, ("Rendering"),
	FIELD_CUSTOM("camera active: ", comp, wants_to_render, 1.1f, true),
	FIELD_CUSTOM("orbital camera: ", comp, orbit_camera, 0.0f, false),
	FIELD_CUSTOM("orbital point: ", comp, target_location, 1.75f, false),
	FIELD_DEFAULT("priority: ", comp, render_priority),
	FIELD_HIDDEN(comp, frame_buffer_target),
	FIELD_CUSTOM("field of view: ", comp, field_of_view, 1.75f, false),
	FIELD_HIDDEN(comp, projection_matrix),
	FIELD_HIDDEN(comp, view_matrix),
	FIELD_HIDDEN(comp, clear_color_buffer),
	FIELD_HIDDEN(comp, clear_depth_buffer),
	FIELD_CUSTOM("target entity: ", comp, target_entity, 2.75f, false),
	FIELD_HIDDEN(comp, near_plane),
	FIELD_HIDDEN(comp, far_plane),
	FIELD_HIDDEN(comp, aspect_ratio),
)

REGISTER_COMPONENT(ShadingLightComponent, ("Rendering"),
	FIELD_CUSTOM("ambient: ", comp, light_ambient, 1.75f, false),
	FIELD_CUSTOM("diffuse: ", comp, light_diffuse, 1.75f, false),
	FIELD_CUSTOM("specular: ", comp, light_specular, 1.75f, false),
	FIELD_CUSTOM("attenuation: ", comp, light_attenuation, 1.75f, false)
)

REGISTER_COMPONENT(RenderComponent, ("Rendering"),
	FIELD_CUSTOM("mesh path: ", comp, mesh, 1.75f, false),
	FIELD_CUSTOM("material path: ", comp, material, 1.75f, false),
	FIELD_CUSTOM("instance draw: ", comp, instance, 1.0f, true),
	FIELD_CUSTOM("use mipmaps: ", comp, mipmap, 0.0f, true),
	FIELD_CUSTOM("load component: ", comp, load, 0.0f, true)
)

REGISTER_COMPONENT(PointLightComponent, ("Rendering", "lighting"),
	FIELD_CUSTOM("color: ", comp, color, 1.75f, false),
	FIELD_CUSTOM("attenuation: ", comp, attenuation, 1.75f, false),
	FIELD_CUSTOM("intensity: ", comp, intensity, 1.75f, false),
	FIELD_CUSTOM("create shadow map: ", comp, shadow_map, 1.75f, false)
)

REGISTER_COMPONENT(DirectionalLightComponent, ("Rendering", "lighting"),
	FIELD_CUSTOM("color: ", comp, color, 1.75f, false),
	FIELD_CUSTOM("intensity: ", comp, intensity, 1.75f, false),
	FIELD_CUSTOM("create shadow map: ", comp, shadow_map, 1.75f, false),
	FIELD_CUSTOM("rotation: ", comp, rotation_euler_do_not_use, 1.75f, false),
	FIELD_CUSTOM("overwrite quaternion rotation: ", comp, overide_quaternion, 1.1f, false),
	FIELD_CUSTOM("update euler rotation: ", comp, get_euler_angles, 1.1f, false),
	FIELD_HIDDEN(comp, rotation_quat),
	)

REGISTER_COMPONENT(MaterialFloatComponent, ("Rendering"),
	FIELD_CUSTOM("material path: ", comp, material, 1.75f, false),
	FIELD_CUSTOM("material location: ", comp, location, 1.75f, false),
	FIELD_CUSTOM("float value: ", comp, value, 1.0f, false),
	FIELD_HIDDEN(comp, prev_value),
	FIELD_CUSTOM("set value: ", comp, set, 1.0f, true),
	FIELD_CUSTOM("update on change: ", comp, continous_update, 0.0f, false),
	)

REGISTER_COMPONENT(MaterialIntComponent, ("Rendering"),
	FIELD_CUSTOM("material path: ", comp, material, 1.75f, false),
	FIELD_CUSTOM("material location: ", comp, location, 1.75f, false),
	FIELD_CUSTOM("float value: ", comp, value, 1.0f, false),
	FIELD_CUSTOM("set value: ", comp, set, 1.0f, false),
	)
REGISTER_COMPONENT(MaterialUniformComponent, ("Rendering"),
	FIELD_CUSTOM("material path: ", comp, type, 1.75f, false),
	FIELD_CUSTOM("material path: ", comp, material, 1.75f, false),
	FIELD_CUSTOM("material location: ", comp, location, 1.75f, false),
	FIELD_CUSTOM("float value: ", comp, value, 1.0f, false),
	FIELD_CUSTOM("set value: ", comp, set, 1.0f, false),
	)

REGISTER_COMPONENT(TestReferenceComponent, ("Default"),
	FIELD_CUSTOM("reference: ", comp, reference, 3.5f, false),
)

REGISTER_COMPONENT(HierarchyComponent, ("Default"),
	FIELD_CUSTOM("parent: ", comp, parent, 3.f, false),
	FIELD_CUSTOM("previous sibling: ", comp, previous_sibling, 3.f, false),
	FIELD_CUSTOM("next sibling: ", comp, next_sibling, 3.f, false),
	FIELD_CUSTOM("child: ", comp, child, 3.f, false),
	)

REGISTER_COMPONENT(SpawnPositionComponent, ("Default"),
	FIELD_CUSTOM("spawn position: ", comp, spawn_position, 1.75f, false),
	)

REGISTER_COMPONENT(ProcessorComponent, ("Default"),
	FIELD_DEFAULT("spawn position: ", comp, processor_id),
	)

REGISTER_COMPONENT(BoundingBoxComponent, ("Rendering, Physics"),
	FIELD_CUSTOM("box offset: ", comp, offset, 1.75f, false),
	FIELD_CUSTOM("collision layers: ", comp, collision_layers, 1.75f, false),
	FIELD_CUSTOM("tag: ", comp, tag, 1.75f, false)
	)

REGISTER_COMPONENT(RigidBodyComponent, ("Physics"),
	FIELD_CUSTOM("position: ", comp, position, 1.75f, false),
	FIELD_CUSTOM("rotation: ", comp, rotation, 1.75f, false),
	FIELD_CUSTOM("linear_velocity: ", comp, linear_velocity, 1.75f, false),
	FIELD_CUSTOM("angular_velocity: ", comp, angular_velocity, 1.75f, false),
	FIELD_CUSTOM("mass: ", comp, mass, 1.75f, false),
	FIELD_CUSTOM("inverse_mass: ", comp, inverse_mass, 1.75f, false),
	FIELD_CUSTOM("restitution: ", comp, restitution, 1.75f, false),
	FIELD_CUSTOM("friction: ", comp, friction, 1.75f, false),
	FIELD_HIDDEN(comp, inverse_inertia_tensor_local),
	FIELD_CUSTOM("recalculate tensor: ", comp, recalculate_inverse_inertia_tensor, 1.f, false),
	FIELD_CUSTOM("dynamic layers: ", comp, dynamic_layers, 1.f, false),
	FIELD_CUSTOM("static layers: ", comp, static_layers, 1.f, false),
	FIELD_CUSTOM("asleep: ", comp, asleep, 1.f, false),
	FIELD_CUSTOM("sleep strikes: ", comp, sleep_strikes, 1.75f, false),
	FIELD_CUSTOM("max sleep strikes: ", comp, max_sleep_strikes, 175.f, false),
	)

REGISTER_COMPONENT(LandscapeComponent, ("Physics"),
	FIELD_CUSTOM("hashed path: ", comp, height_map_path, 1.75f, false),
	FIELD_HIDDEN(comp, map),
	FIELD_CUSTOM("static_layers: ", comp, static_layers, 1.75f, false),
	FIELD_CUSTOM("dynamic_layers: ", comp, dynamic_layers, 1.75f, false),
	FIELD_CUSTOM("collision_layers: ", comp, collision_layers, 1.75f, false),
	FIELD_CUSTOM("max_height: ", comp, max_height, 1.75f, false),
	FIELD_CUSTOM("has_loaded_height_map: ", comp, has_loaded_height_map, 1.75f, false)
)

REGISTER_COMPONENT(UIMaterialComponent, ("Render"),
	FIELD_CUSTOM("hashed path: ", comp, material, 1.75f, false)
)

REGISTER_COMPONENT(CursorComponent, ("Render"),
	FIELD_CUSTOM("active: ", comp, active, 1.75f, false)
)
REGISTER_COMPONENT(UISelectableComponent, ("Render"),
	FIELD_HIDDEN(comp, selectable)
)

REGISTER_COMPONENT(UITextComponent, ("Render"),
	FIELD_CUSTOM("hashed path: ", comp, material, 1.75f, false),
	FIELD_CUSTOM("text: ", comp, text, 1.75f, false),
	FIELD_CUSTOM("position: ", comp, position, 1.75f, false),
	FIELD_CUSTOM("size: ", comp, size, 1.75f, false),
	FIELD_CUSTOM("font_size: ", comp, font_size, 1.75f, false),
	FIELD_CUSTOM("color: ", comp, color, 1.75f, false),
	FIELD_CUSTOM("order: ", comp, order, 1.75f, false)
)

REGISTER_COMPONENT(UIRectComponent, ("Render"),
	FIELD_CUSTOM("hashed path: ", comp, material, 1.75f, false),
	FIELD_CUSTOM("position: ", comp, position, 1.75f, false),
	FIELD_CUSTOM("size: ", comp, size, 1.75f, false),
	FIELD_CUSTOM("uv_offset: ", comp, uv_offset, 1.75f, false),
	FIELD_CUSTOM("color: ", comp, color, 1.75f, false),
	FIELD_CUSTOM("order: ", comp, order, 1.75f, false)
)

REGISTER_COMPONENT(UIMenuItemStatusComponent, ("Render"),
	FIELD_CUSTOM("pressed: ", comp, pressed, 1.75f, false),
	FIELD_CUSTOM("hovered: ", comp, hovered, 1.75f, false)
)
REGISTER_COMPONENT(UIColorChangeComponent, ("Render"),
	FIELD_CUSTOM("active color: ", comp, active, 1.75f, false),
	FIELD_CUSTOM("inactive color: ", comp, inactive, 1.75f, false)
)
