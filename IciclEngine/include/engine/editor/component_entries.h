#include <engine/game/components.h>
#include <engine/game/component_data.h>
#include <engine/editor/componentdata_macros.h>

REGISTER_COMPONENT(TestComponent, ("Test Category"), FIELD_DEFAULT("test: ", comp, test))
REGISTER_COMPONENT(NameComponent, ("Default"),
	FIELD_CUSTOM("entity name: ", comp, hashed_name, 2.0f, false),
	FIELD_READONLY_CUSTOM("entity id: ", comp, entity, 1.25f, false)
)

REGISTER_COMPONENT(TransformDynamicComponent, ("Transform", "Rendering"),
	FIELD_CUSTOM("position: ", comp, position, 2.f, false),
	FIELD_CUSTOM("scale: ", comp, scale, 2.f, false),
	FIELD_CUSTOM("rotation: ", comp, rotation_euler_do_not_use, 2.f, false),
	FIELD_CUSTOM("update euler rotation: ", comp, get_euler_angles, 1.1f, true),
	FIELD_CUSTOM("overwrite quaternion rotation: ", comp, overide_quaternion, 0.f, false),
	FIELD_HIDDEN(comp, rotation_quat),
	FIELD_HIDDEN(comp, model_matrix),
	)


	REGISTER_COMPONENT(MeshComponent, ("Rendering"),
		FIELD_DEFAULT("mesh loaded: ", comp, loaded),
		FIELD_CUSTOM("mesh path: ", comp, hashed_path, 2.25f, false)
	)

	REGISTER_COMPONENT(TextureComponent, ("Rendering"),
		FIELD_DEFAULT("texture loaded: ", comp, loaded),
		FIELD_CUSTOM("texture path: ", comp, hashed_path, 2.25f, false)
	)

	REGISTER_COMPONENT(CameraComponent, ("Rendering"),
		FIELD_CUSTOM("camera active: ", comp, wants_to_render, 1.1f, true),
		FIELD_CUSTOM("orbital camera: ", comp, orbit_camera, 0.0f, false),
		FIELD_CUSTOM("orbital point: ", comp, target_location, 2.0f, false),
		FIELD_DEFAULT("priority: ", comp, render_priority),
		FIELD_CUSTOM("buffer: ", comp, frame_buffer_target, 2.25f, false),
		FIELD_DEFAULT("field of view: ", comp, field_of_view),
		FIELD_HIDDEN(comp, projection_matrix),
		FIELD_HIDDEN(comp, view_matrix),
		FIELD_HIDDEN(comp, clear_color_buffer),
		FIELD_HIDDEN(comp, clear_depth_buffer),
		FIELD_CUSTOM("target entity: ", comp, target_entity, 3.f, false),
		FIELD_HIDDEN(comp, near_plane),
		FIELD_HIDDEN(comp, far_plane),
		FIELD_HIDDEN(comp, aspect_ratio),
		)
