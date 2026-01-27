//#include "scene.h"
#include <engine/editor/scene.h>

#include <engine/game/components.h>
#include <engine/editor/scene_object.h>
#include <imgui-docking/imgui.h>
#include <fstream>
#include <engine/resources/scene_object_registry.h>
#include <unordered_map>
#include <engine/game/systems.h>
#include <engine/editor/systems_registry.h>

Scene::Scene()
{
	name = "new scene";
}
Scene::Scene(std::shared_ptr<Scene> a_scene) // do I ever use this??
{
	auto scene = a_scene;
	auto scene_objects = scene->get_scene_objects();
	for (size_t i = 0; i < scene_objects.size(); i++)
	{
		scene_objects[i]->start_runtime(shared_from_this());
	}
}
Scene::~Scene()
{
	for (size_t i = 0; i < scene_objects.size(); i++)
	{
		scene_objects[i]->stop_runtime();
	}
	
}

void Scene::reset()
{
	registry.clear();
	root_scene_objects.clear();
	scene_objects.clear();
	systems.clear();
	SceneObjectRegistry& id_registry = SceneObjectRegistry::instance();
	id_registry.clear_registry();
}

bool Scene::save(std::string a_path)
{
	json j = json::object();
	j["name"] = name;
	j["next index"] = next_index;
	j["root objects"] = json::array();
	for (auto& scene_object : root_scene_objects)
	{
		j["root objects"].push_back(scene_object->save());
	}

	j["systems"] = json::array();
	for (auto& system : systems)
	{
		json j_sys = json::object();
		j_sys["order"] = system->get_order();
		j_sys["name"] = system->get_name();
		j_sys["enabled"] = system->get_enabled();
		j_sys["physics"] = system->get_physics_frames_only();
		j["systems"].push_back(j_sys);
	}

	std::ofstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to save at: {}", a_path);
		return false;
	}

	file << j.dump(4);
	return true;
}

bool Scene::load(std::string a_path, bool clear_registry)
{
	std::ifstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to load at: {}", a_path);
		return false;
	}
	SceneObjectRegistry& id_registry = SceneObjectRegistry::instance();
	if (clear_registry)
	{
		registry.clear();
		for (auto [entity, name_comp] : registry.view<EntityComponent>().each())
		{
			PRINTLN("ENTITIES STILL HERE");
		}
		id_registry.clear_registry();
	}
	//SceneObjectRegistry::instance().clear_registry();

	json j;
	file >> j;
	root_scene_objects.clear(); // order is important here I think
	scene_objects.clear();
	systems.clear();
	j["name"] = name;
	j["next index"] = next_index;

	if (j["root objects"].is_array())
	{
		for (auto& root_obj : j["root objects"])
		{
			root_scene_objects.push_back(SceneObject::load(root_obj, shared_from_this()));
			//scene_objects.push_back(root_scene_objects.back());
		}
	}

	// we've loaded these ... now we need to register their IDs...
	std::unordered_map<uint32_t, uint32_t> id_remapping; // old -> new
	for (auto& scene_object : scene_objects)
	{
		auto& component_datas = scene_object->get_component_datas();
		for (const auto& component : component_datas)
		{
			if (component->get_type() == typeid(EntityComponent))
			{
				//now get the ids and stuff.
				TrueID& id = id_registry.get_new_ID();
				const auto& fields = component->get_registered_field_info();
				for (auto& field : fields)
				{
					if (field.type == typeid(EntityReference))
					{
						EntityReference* ent_ref = static_cast<EntityReference*>(field.value_ptr);
						id_remapping[ent_ref->scene_object] = id.true_id;
						ent_ref->scene_object = id.true_id;
						if (!scene_object->try_set_id(id.true_id))
						{
							PRINTLN("failed setting the scene object ID");
						}
						break;
					}
				}
				break;
			}
		}
	}
	if (runtime)
	{
		for (auto& scene_object : scene_objects)
		{
			scene_object->start_runtime(shared_from_this());
		}
	}

	/////////////////////////////////////////
	// Now recalculate all references.
	std::vector<EntityReference*> entity_references;
	entity_references.reserve(scene_objects.size() * 2);
	for (auto& scene_object : scene_objects)
	{
		auto& component_datas = scene_object->get_component_datas();
		for (const auto& component : component_datas)
		{
			const auto& fields = component->get_registered_field_info();
			if (component->get_type() == typeid(EntityComponent))
			{
				continue;
			}
			for (auto& field : fields)
			{
				if (field.type == typeid(EntityReference))
				{
					EntityReference* ent_ref = static_cast<EntityReference*>(field.value_ptr);
					if (id_remapping.contains(ent_ref->scene_object))
					{
						TrueID* true_id = id_registry.get_registred_ID(id_remapping[ent_ref->scene_object]);
						if (true_id->true_id == 0)
						{
							PRINTLN("the true id was 0 when loading!!!");
						}
						ent_ref->scene_object = true_id->true_id;
						ent_ref->entity = true_id->entity;
					}
					else
					{
						PRINTLN("loaded entity reference did not reference a valid entity");
					}
					//entity_references.push_back(static_cast<EntityReference*>(field.value_ptr));
				}
			}
		}
	}
	for (auto reference : entity_references)
	{
		PRINTLN("test print {}", reference->scene_object);
	}
	if (j.contains("systems"))
	{
		auto& sys_factory = SystemsFactory::instance();
		auto& sys_reg = SystemsRegistry::instance();
		for (auto& j_sys : j["systems"])
		{
			if (j_sys.contains("name") && j_sys.contains("order") && j_sys.contains("physics") && j_sys.contains("enabled"))
			{
				if (sys_factory.has_factory(j_sys["name"]))
				{
					auto sys = sys_factory.create_system(j_sys["name"]);
					sys->set_name(j_sys["name"].get<std::string>());
					sys->set_order(j_sys["order"].get<int>());
					sys->set_enabled(j_sys["enabled"].get<bool>());
					sys->set_only_on_physics(j_sys["physics"].get<bool>());
					add_system(sys);
				}
			}
		}
		reorder_systems();
	}

	return true;
}

entt::handle Scene::create_entity(const std::string a_name)
{
	auto entity = registry.create();
	auto& handle = entity_handles.emplace_back(entt::handle{ registry, entity });
	//registry.emplace<EntityComponent>(entity, "a_name");
	return handle;
}

void Scene::destroy_entity(entt::entity a_entity)
{
	if (runtime)
	{
		uint32_t ent = (uint32_t)a_entity;
		if (registry.valid(a_entity))
		{
			registry.destroy(a_entity);
		}
	}
}

void Scene::destroy_scene_object(std::weak_ptr<SceneObject> a_scene_object)
{
	if (auto scene_object = a_scene_object.lock())
	{
		if (runtime)
		{
			destroy_entity(scene_object->get_entity());
		}
		auto children = scene_object->get_children();
		for (size_t i = 0; i < children.size(); i++)
		{
			destroy_scene_object(children[i]);
		}
		for (size_t i = 0; i < scene_objects.size(); i++)
		{
			if (scene_object.get() == scene_objects[i].get())
			{
				scene_objects.erase(scene_objects.begin() + i);
				break;
			}
		}
		for (size_t i = 0; i < root_scene_objects.size(); i++)
		{
			if (scene_object.get() == root_scene_objects[i].get())
			{
				root_scene_objects.erase(root_scene_objects.begin() + i);
				break;
			}
		}
	}

}

void Scene::add_scene_object(std::shared_ptr<SceneObject> a_scene_object)
{
	scene_objects.push_back(a_scene_object);
}

void Scene::parent_scene_object(std::weak_ptr<SceneObject> a_parent_scene_object, std::weak_ptr<SceneObject> a_target_scene_object)
{
	// TODO: actually add the entityref stuff to the child and parent when adding child -> I think done?
	if (auto scene_object = a_target_scene_object.lock())
	{
		auto parent_scene_object = a_parent_scene_object.lock();
		if (parent_scene_object->parent.lock().get() == scene_object.get()) // this does not fix the problem ...
		{
			// this needs to be recursive loop down to see that the parent is never scene_object
			return;
		}
		// first get the parent of this scene_object...
		if (auto old_parent = scene_object->parent.lock())
		{
			orphan_scene_object(scene_object);
		}
		if (parent_scene_object) // now notify the new parent.
		{
			//parent_scene_object->remove_child(scene_object);
			// add as child to parent.
			parent_scene_object->add_child(a_target_scene_object);

			// remove from root if it is root object.
			for (size_t i = 0; i < root_scene_objects.size(); i++)
			{
				if (scene_object.get() == root_scene_objects[i].get())
				{
					root_scene_objects.erase(root_scene_objects.begin() + i);
					break;
				}
			}
		}
		else
		{
			PRINTLN("NO PARENT!!");
		}
	}
}

void Scene::orphan_scene_object(std::weak_ptr<SceneObject> a_target_scene_object)
{
	// if it's dragged into nothing.. or pressed button to orphan...

	// if not already root object
	if (auto scene_object = a_target_scene_object.lock())
	{
		// get parent scene object, tell to remove child.
		if (!scene_object->parent.expired()) // could look through all root objects too
		{
			if (auto parent_object = scene_object->parent.lock())
			{
				parent_object->remove_child(scene_object);
				root_scene_objects.push_back(scene_object);
				scene_object->parent.reset();
			}
			//scene_object->parent.reset();
		}
		// no parent, no care
		
	}

}

std::weak_ptr<SceneObject> Scene::get_scene_object_by_ID(uint32_t a_id)
{
	for (auto& scene_object : scene_objects)
	{
		if (scene_object->scene_object_id == a_id)
		{
			return scene_object;
		}
	}
	return std::weak_ptr<SceneObject>();
}

std::weak_ptr<SceneObject> Scene::new_scene_object(const std::string a_name, bool as_root_object)
{
	//auto scene_object = SceneObject(a_name, shared_from_this());
	//std::weak_ptr<SceneObject> scene_object = scene_objects.emplace_back(std::make_shared<SceneObject>(a_name, shared_from_this()));
	//return scene_object;
	auto scene_object = std::make_shared<SceneObject>(a_name, shared_from_this());
	if (as_root_object)
	{
		root_scene_objects.push_back(scene_object);
	}
	scene_objects.push_back(scene_object);
	if (runtime)
	{
		scene_object->start_runtime(shared_from_this());
	}
	return scene_object;
}

//bool Scene::add_scene_object(SceneObject& a_scene_object)
//{
//	root_scene_objects.push_back(std::make_shared<SceneObject>(a_scene_object));
//	return true;
//}

const std::vector<std::shared_ptr<SceneObject>> Scene::get_scene_objects()
{
	return scene_objects;
}
const std::vector<std::shared_ptr<SceneObject>> Scene::get_root_scene_objects()
{
	return root_scene_objects;
}

//void Scene::draw_imgui()
//{
//	for (size_t i = scene_objects.size(); i > 0; i--)
//	{
//		if (runtime && !scene_objects[i-1].get()->has_valid_entity())
//		{
//			scene_objects.erase(scene_objects.begin() + (i-1));
//			PRINTLN("removing scene_object, as it has no entity");
//		}
//		else
//		{
//			//if (ImGui::TreeNode(scene_objects[i - 1].get()->get_name().c_str()))
//			//{
//				scene_objects[i - 1].get()->draw_components();
//			//}
//		}
//	}
//}

void Scene::start_runtime()
{
	if (!runtime)
	{
		runtime = true;
		for (auto scene_object : scene_objects)
		{
			scene_object->start_runtime(shared_from_this());
		}
		// have to tell all scene objects to fix their refs....
		for (auto scene_object : scene_objects)
		{
			scene_object->get_entity_references();
		}
	}

	//for (size_t i = 0; i < scene_objects.size(); i++)
	//{
	//	scene_objects[i]->start_runtime(shared_from_this());
	//}
}

void Scene::stop_runtime()
{
	if (runtime)
	{
		runtime = false;
		for (auto scene_object : root_scene_objects)
		{
			scene_object->stop_runtime();
		}
		registry.clear();
	}
}

void Scene::add_system(std::shared_ptr<SystemBase> a_system)
{
	uint32_t order = a_system->get_order();
	size_t insertion_index = 0;
	for (; insertion_index < systems.size(); ++insertion_index)
	{
		if (systems[insertion_index]->get_order() >= order)
		{
			break;
		}
	}
	systems.insert(systems.begin() + insertion_index, a_system);
}

void Scene::remove_system(std::weak_ptr<SystemBase> a_system)
{
	if (auto system = a_system.lock())
	{
		auto* system_ptr = system.get();
		size_t erase_index = 0;
		for (; erase_index < systems.size(); ++erase_index)
		{
			if (systems[erase_index].get() == system_ptr)
			{
				systems.erase(systems.begin() + erase_index);
			}
		}
	}
}

void Scene::reorder_systems()
{
	std::sort(systems.begin(), systems.end(), []
	(const std::shared_ptr<SystemBase>system_a, const std::shared_ptr<SystemBase> system_b)
		{
			return *system_a < *system_b;
		});
}

std::vector<std::shared_ptr<SystemBase>>& Scene::get_systems()
{
	return systems;
}
