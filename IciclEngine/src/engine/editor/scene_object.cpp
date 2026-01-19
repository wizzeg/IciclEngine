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
	component_datas.emplace_back(std::make_unique<ComponentData<EntityComponent>>(EntityComponent{ a_name.c_str(), {entt::null, scene_object_id} }));
	component_types.emplace_back(typeid(EntityComponent));
	component_datas.emplace_back(std::make_unique<ComponentData<HierarchyComponent>>(HierarchyComponent{ }));
	component_types.emplace_back(typeid(HierarchyComponent));
}

SceneObject::SceneObject(const std::string a_name, std::weak_ptr<SceneObject> a_parent, std::weak_ptr<Scene> a_scene, bool a_register_id) // DO NOT USE
	: name(a_name), parent(a_parent), scene(a_scene)
{
	PRINTLN("DO NOT USE THIS SCENE OBJECT CONSTRUCTOR YET");
	if (a_register_id)
	{
		scene_object_id = SceneObjectRegistry::instance().get_new_ID().true_id;
	}
	component_datas.emplace_back(std::make_unique<ComponentData<EntityComponent>>(EntityComponent{ a_name.c_str(), {entt::null, scene_object_id} }));
	component_types.emplace_back(typeid(EntityComponent));
	component_datas.emplace_back(std::make_unique<ComponentData<HierarchyComponent>>(HierarchyComponent{ }));
	component_types.emplace_back(typeid(HierarchyComponent));
}

void SceneObject::add_child(std::weak_ptr<SceneObject> a_child)
{
	if (auto new_child = a_child.lock())
	{
		auto parent_ref = get_component<HierarchyComponent>();
		auto parent_ent = get_component<EntityComponent>();
		auto new_child_ref = new_child->get_component<HierarchyComponent>();
		auto new_child_ent = new_child->get_component<EntityComponent>();
		new_child_ref->previous_sibling = EntityReference{};
		new_child_ref->previous_sibling = EntityReference{};
		for (size_t i = 0; i < children.size(); i++)
		{
			if (auto old_child = children[i].lock()) 
			{
				auto old_child_ent = old_child->get_component<EntityComponent>();
				if (old_child_ent->entity.scene_object == parent_ref->child.scene_object || (runtime && (old_child_ent->entity.entity == parent_ref->child.entity)))
				{
					HierarchyComponent* old_child_ref;
					EntityComponent* old_child_ent;
					// first setup the new childs ent ref
					if (parent_ent && parent_ref && new_child_ref && new_child_ent)
					{
						// first look for the child among children
						for (auto& child : children)
						{
							if (auto child_lock = child.lock())
							{
								auto child_ent = child_lock->get_component<EntityComponent>();

								if (child_ent)
								{
									if (child_ent->entity.scene_object == parent_ref->child.scene_object || (runtime && (child_ent->entity.entity == parent_ref->child.entity)))
									{
										// this is the child the parent refs too
										old_child_ref = child_lock->get_component<HierarchyComponent>();
										old_child_ent = child_lock->get_component<EntityComponent>();
										if (old_child_ent && old_child_ref)
										{
											old_child_ref->previous_sibling = new_child_ent->entity;
											new_child_ref->next_sibling = old_child_ent->entity;
										}
										break;
									}
								}

								//if (child_lock.get() == new_child.get()) // this makes no sense, we want to update child references
								//{
								//	// this is the child the parent refs too
								//	old_child_ref = child_lock->get_component<HierarchyComponent>();
								//	old_child_ent = child_lock->get_component<EntityComponent>();
								//	if (old_child_ent && old_child_ref)
								//	{
								//		old_child_ref->previous_sibling = new_child_ent->entity;
								//		new_child_ref->next_sibling = old_child_ent->entity;
								//	}
								//}
							}
						}
						// it's an only cild.
					}
					// then link old child to the new child
					// set the new child to to child for parent
					//return; // NO RETURN ... we have to tell tell the first sibling there's new first sibling
				}
			}
		}
		// need to tell child that it has a new parent
		parent_ref->child = new_child_ent->entity;
		new_child_ref->parent = parent_ent->entity;
		new_child->parent = shared_from_this();
		children.insert(children.begin(), new_child);
	}
	

	PRINTLN("child acquired, I have this many children now: {}", children.size());
}
void SceneObject::orphan()
{
	parent.reset();
	if (HierarchyComponent* comp = get_component<HierarchyComponent>())
	{
		comp->parent = EntityReference{};
		comp->next_sibling = EntityReference{};
		comp->previous_sibling = EntityReference{};

	} // scene needs to me made aware that there's new root component too
	// I think the scene will do that
}
void SceneObject::remove_child(std::weak_ptr<SceneObject> a_child)
{
	if (auto orphan_child = a_child.lock())
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			if (auto child = children[i].lock())
			{
				if (orphan_child.get() == child.get())
				{
					// must also go into the Hierarchy component to remove child, and restructure parent heirarchy component.
					// if this childs entt/true id == this parents child entt/true set the sibling as the parents child
					// and the siblings previous sibling must also be changed...
					if (auto child_name = child->get_component<EntityComponent>())
					{
						if (auto child_ref = child->get_component<HierarchyComponent>())
						{
							if (auto parent_ref = get_component<HierarchyComponent>())
							{
								if (auto locked_scene = scene.lock())
								{
									//if (runtime)// wait we still have to do the big one... this is IF NOT EDITOR
									//{
									//	// remove and link siblings
									//	if (HierarchyComponent* next_sibling_ref = locked_scene->get_registry().try_get<HierarchyComponent>(child_ref->next_sibling.entity))
									//	{
									//		next_sibling_ref->previous_sibling = child_ref->previous_sibling;
									//	}
									//	if (HierarchyComponent* prev_sibling_ref = locked_scene->get_registry().try_get<HierarchyComponent>(child_ref->previous_sibling.entity))
									//	{
									//		prev_sibling_ref->next_sibling = child_ref->next_sibling;
									//	}
									//	// replace child of parent
									//	if (parent_ref->child.entity == child_name->entity.entity) 
									//	{
									//		if (EntityComponent* name_comp = locked_scene->get_registry().try_get<EntityComponent>(child_ref->next_sibling.entity))
									//		{
									//			parent_ref->child = name_comp->entity; // tell parent new child
									//		}
									//	}
									//}
									//else
									{
										// have to look through children to find the next sibling.
										for (auto next_child : children)
										{
											if (auto next_child_lock = next_child.lock())
											{
												if (auto next_child_entity = next_child_lock->get_component<EntityComponent>())
												{
													if (next_child_entity->entity.scene_object == child_ref->next_sibling.scene_object)
													{// found match
														if (auto next_child_ref = next_child_lock->get_component<HierarchyComponent>())
														{
															next_child_ref->previous_sibling = child_ref->previous_sibling;
														}
														break;
													}
												}
												else
												{
													PRINTLN("uh oh, something doesn't have EntityComponent")
												}
											}
										}

										for (auto prev_child : children)
										{
											if (auto prev_child_lock = prev_child.lock())
											{
												if (auto prev_child_entity = prev_child_lock->get_component<EntityComponent>())
												{
													if (prev_child_entity->entity.scene_object == child_ref->previous_sibling.scene_object)
													{// found match
														if (auto prev_child_ref = prev_child_lock->get_component<HierarchyComponent>())
														{
															prev_child_ref->next_sibling = child_ref->next_sibling;
														}
														break;
													}
												}
												else
												{
													PRINTLN("uh oh, something doesn't have EntityComponent")
												}
											}
										}
										if (parent_ref->child.scene_object == child_name->entity.scene_object || (runtime && (parent_ref->child.entity == child_name->entity.entity)))
										{
											parent_ref->child = child_ref->next_sibling;
										}
									}
								}
							}
						}
					}
					orphan_child->orphan();
					children.erase(children.begin() + i);
					return;
				}
			}
		}
	}
}

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
			if (component_datas[i]->get_type() == typeid(EntityComponent))// actually, kinda needed, because I need to init the TrueID
			{
				EntityComponent new_name_comp(name.c_str(), {entity_handle.entity(), scene_object_id});

				auto comp = static_cast<ComponentData<EntityComponent>*>(component_datas[i].get());
				comp->set_new_component_value(EntityComponent(new_name_comp));
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

					//std::shared_ptr<SceneObject> child = SceneObject::load(j_child, a_scene);
					//scene->add_scene_object(child);
					//scene_object->children.push_back(child);
					scene_object->add_child(SceneObject::load(j_child, a_scene));
			}
		}
		scene->add_scene_object(scene_object);
	}
	return scene_object;
}
