#include <engine/editor/scene_object.h>
#include <engine/editor/scene.h>
#include <engine/utilities/macros.h>
#include <engine/editor/field_serialization_registry.h>
#include <engine/utilities/serialization.h>
#include <engine/editor/component_factory.h>
#include <engine/resources/scene_object_registry.h>
#include <typeindex>


SceneObject::SceneObject(const std::string a_name, std::weak_ptr<Scene> a_scene, bool a_register_id)
	: name(a_name), scene(a_scene)
{
	/////////////// get true_id
	if (a_register_id)
	{
		scene_object_id = SceneObjectRegistry::instance().get_new_ID().true_id;
	}
	component_datas.emplace_back(std::make_unique<ComponentData<NameComponent>>(NameComponent{ a_name.c_str(), {entt::null, scene_object_id} }));
	component_types.emplace_back(typeid(NameComponent));
}

SceneObject::SceneObject(const std::string a_name, std::weak_ptr<SceneObject> a_parent, std::weak_ptr<Scene> a_scene, bool a_register_id) // DO NOT USE
	: name(a_name), parent(a_parent), scene(a_scene)
{
	PRINTLN("DO NOT USE THIS SCENE OBJECT CONSTRUCTOR YET");
	if (a_register_id)
	{
		scene_object_id = SceneObjectRegistry::instance().get_new_ID().true_id;
	}
	component_datas.emplace_back(std::make_unique<ComponentData<NameComponent>>(NameComponent{ a_name.c_str(), {entt::null, scene_object_id} }));
	component_types.emplace_back(typeid(NameComponent));
}

void SceneObject::add_child(std::weak_ptr<SceneObject> a_child) // need to add remove_child aswell, and give entity children if added childre, and tell children they have a new parent -> make these into components
{
	if (auto a_shared = a_child.lock())
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			if (auto shared = children[i].lock())
			{
				if (a_shared == shared)
				{
					return;
				}
			}
		}
	}
	children.push_back(a_child);
	PRINTLN("child acquired, I have this many children now: {}", children.size());
};

//void SceneObject::draw_components()
//{
//	bool minus_one = false;
//	for (size_t i = 0; i < component_datas.size(); i++)
//	{
//		if (minus_one)
//		{
//			i--;
//			minus_one = false;
//		}
//		if (!component_datas[i]->draw_imgui(entity_handle, runtime)) // has component?
//		{
//			if (runtime)
//			{
//				component_datas.erase(component_datas.begin() + i); // this is not great, should be removing in a smarter way, this will rearrange multiple times
//				minus_one = true;
//			}
//		}
//	}
//}

//void SceneObject::destroy_scene_object()
//{
//	if (auto shared_scene = scene.lock())
//	{
//		shared_scene->destroy_scene_object(shared_from_this());
//	}
//}

entt::handle SceneObject::get_entity_handle() const
{
	return entity_handle;
}

entt::handle SceneObject::to_runtime(std::weak_ptr<Scene> a_scene) // TODO ... need to check if field has entity reference
{
	runtime = true;
	//PRINTLN("runtime started");
	if (auto scene = a_scene.lock())
	{
		entity_handle = scene->create_entity(name);
		TrueID* id = SceneObjectRegistry::instance().get_registred_ID(scene_object_id);
		if (id)
		{
			id->initialized = true;
			id->entity = entity_handle.entity();
		}
		else
		{
			PRINTLN("TrueID could not be found and does not contain the entity reference");
		}
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			component_datas[i]->to_runtime(entity_handle);
			// old, I think I don't need to do it that way at all... need to do for all entity references
			if (component_datas[i]->get_type() == typeid(NameComponent))// actually, kinda needed, because I need to init the TrueID
			{
				NameComponent new_name_comp(name.c_str(), {entity_handle.entity(), scene_object_id});

				auto comp = static_cast<ComponentData<NameComponent>*>(component_datas[i].get());
				comp->set_new_component_value(NameComponent(new_name_comp));
			}
			const auto& fields = component_datas[i]->get_registered_field_info();
			std::type_index type = typeid(EntityReference);
			//for (auto& field : fields) // this is weird... I don't think I should do this?
			//{
			//	if (field.type == type)
			//	{
			//		EntityReference* ref = static_cast<EntityReference*>(field.value_ptr);
			//		ref->entity = entity_handle.entity();
			//		ref->scene_object = scene_object_id;
			//	}
			//}
		}
		//for (auto child : children)
		//{
		//	if (auto c = child.lock())
		//	{
		//		c->to_runtime(a_scene);
		//	}
		//}
	}

	return entity_handle;
}

bool SceneObject::check_valid(size_t a_index)
{
	if (runtime && component_datas.size() > a_index)
	{
		if (component_datas[a_index]->is_valid())
			return true;

		remove_component_data(a_index);
		return false;
	}
	return true;
}

void SceneObject::scene_ended()
{
	runtime = false;
}

std::weak_ptr<Scene> SceneObject::get_scene()
{
	return scene;
}

json SceneObject::save()
{
	json j = json::object();
	j["name"] = name;
	j["children"] = json::array();
	j["components"] = json::array();
	for (auto& component : component_datas)
	{
		json j_comp = json::object();
		j_comp["type"] = component->get_name();
		j_comp["fields"] = json::array();
		
		auto fields = component->get_registered_field_info();
		for (auto& field :fields)
		{
			json j_field = json::object();
			if (field.name.empty())
			{
				PRINTLN("messed up during serialization, field name is empty");
			}
			serialize_field(j_field, field.name, field.type, field.value_ptr);
			j_field["name"] = field.name;
			j_field["type"] = field.type.name();
			j_comp["fields"].push_back(j_field);
		}

		j["components"].push_back(j_comp);

	}
	
	for (auto& child : children)
	{
		if (auto c = child.lock())
		{
			j["children"].push_back(c->save());
		}
	}
	return j;
}

std::shared_ptr<SceneObject> SceneObject::load(const json& a_j, std::weak_ptr<Scene> a_scene)
{
	std::string temp_name = " ";
	if (a_j.contains("name"))
	{
		temp_name = a_j["name"].get<std::string>();
	}
	std::shared_ptr<SceneObject> scene_object = std::make_shared<SceneObject>(temp_name, a_scene, false);

	if (a_j.contains("components"))
	{
		for (const auto& comp_j_obj : a_j["components"])
		{
			if (comp_j_obj.is_object())
			{
				std::string obj_name = comp_j_obj["type"];
				auto& j_fields = comp_j_obj["fields"];
				ComponentFactory::instance().create_component(obj_name, scene_object.get());

				if (scene_object->component_datas.size() > 0 && j_fields.is_array())
				{
					auto& comp_data = scene_object->component_datas.back();
					auto fields = comp_data->get_registered_field_info();

					for (auto& field : fields)
					{
						std::string field_type_name = field.type.name();
						std::string field_name = field.name;
						for (const auto& j_field : j_fields)
						{
							if (j_field.contains("name"))
							{
								if (j_field["name"] == field_name)
								{
									deserialize_field(j_field, field_name, field.type, field.value_ptr);

								}
							}
						}
					}
				}
			}
		}
	}
	if (auto scene = a_scene.lock())
	{
		if (a_j.contains("children"))
		{
			for (auto& j_child : a_j["children"])
			{

					std::shared_ptr<SceneObject> child = SceneObject::load(j_child, a_scene);
					//scene->add_scene_object(child);
					scene_object->children.push_back(child);
			}
		}
		scene->add_scene_object(scene_object);
	}
	return scene_object;
}
