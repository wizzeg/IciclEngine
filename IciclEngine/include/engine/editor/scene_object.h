#pragma once

#include <string>
#include <engine/utilities/entt_modified.h>
#include <engine/game/component_data.h>
#include <engine/utilities/macros.h>

class Scene;

class SceneObject : public std::enable_shared_from_this<SceneObject>
{
	friend class SceneObject;
	friend class Scene;
private:

	void add_child(std::weak_ptr<SceneObject> a_child);
	void orphan();
	void remove_child(std::weak_ptr<SceneObject> a_child);
	
	std::string name = "";
	std::weak_ptr<SceneObject> parent;
	std::vector<std::weak_ptr<SceneObject>> children;
	std::weak_ptr<Scene> scene;

	std::vector<std::unique_ptr<ComponentDataBase>> component_datas;
	std::vector<std::type_index> component_types;

	entt::handle entity_handle;
	uint32_t scene_object_id = 0;

	bool runtime = false;
	bool ui_opened = false;
	bool original = true;
public:

	SceneObject(SceneObject&&) = default;
	SceneObject& operator=(SceneObject&&) = default;
	//SceneObject() { name = "none"; }
	SceneObject(const std::string a_name, std::weak_ptr<Scene> a_scene, bool a_register_id = true);
	SceneObject(const std::string a_name, std::weak_ptr<SceneObject> a_parent, std::weak_ptr<Scene> a_scene, bool a_register_id = true);
	~SceneObject() { PRINTLN("Scene Object Destroyed: {}", name); }// if this has parent, add all children to it, otherwise I don't know... 
	void stop_runtime();
	entt::entity get_entity() { return entity_handle.entity(); }
	bool has_valid_entity() { return entity_handle.valid(); }
	std::weak_ptr<Scene> get_scene();
	bool try_set_id(uint32_t a_id)
	{
		if (scene_object_id == 0)
		{
			scene_object_id = a_id;
			return true;
		}
		return false;
	}

	json save();
	static std::shared_ptr<SceneObject> load(const json& a_j, std::weak_ptr<Scene> a_scene);

	//void destroy_scene_object();

	entt::handle get_entity_handle() const; ///////////////////// REMOVE USAGE OF ENTITY (use entt::handle)

	template <typename TcomponentData, typename TComponent>
	bool add_component_data(TComponent&& a_component) // deprecated
	{
		static_assert(std::is_base_of<ComponentData<std::decay_t<TComponent>>, TcomponentData>::value, "TcomponentData must derive from ComponentData<T> with matching TComponent");
		//static_assert(std::is_standard_layout<std::decay_t<TComponent>>::value, "TComponent must be a standard-layout type (e.g no inheritance)");
		//static_assert(std::is_trivial<std::decay_t<TComponent>>::value, "TComponent must be trivial (e.g. no smart pointers)");
		std::type_index t_type = typeid(std::decay_t<TComponent>);
		for (size_t i = 0; i < component_types.size(); i++)
		{
			if (t_type == component_types[i])
			{
				return false;
			}
		}
		component_types.push_back(t_type);
		component_datas.emplace_back(std::make_unique<TcomponentData>(std::forward<TComponent>(a_component)));
	}

	template <typename TComponent>
	ComponentData<TComponent>* add_component() // new variant
	{
		bool unique_component = true;
		for (const auto& comp_data : component_datas)
		{
			if (comp_data->get_type() == typeid(TComponent))
			{
				return nullptr;
			}
		}
		component_datas.emplace_back(std::make_unique<ComponentData<TComponent>>(TComponent{}));
		if (runtime)
		{
			component_datas.back()->to_runtime(entity_handle);
		}
		return static_cast<ComponentData<TComponent>*>(component_datas.back().get());
	}

	template <typename TComponent>
	ComponentData<TComponent>* add_component(const TComponent& component)
	{
		bool unique_component = true;
		for (const auto& comp_data : component_datas)
		{
			if (comp_data->get_type() == typeid(TComponent))
			{
				return nullptr;
			}
		}
		component_datas.emplace_back(std::make_unique<ComponentData<TComponent>>(component));
		if (runtime)
		{
			component_datas.back()->to_runtime(entity_handle);
		}
		return static_cast<ComponentData<TComponent>*>(component_datas.back().get());
	}

	template <typename TComponent>
	ComponentData<TComponent>* add_or_replace_component(const TComponent& component)
	{
		for (const auto& comp_data : component_datas)
		{
			if (comp_data->get_type() == typeid(TComponent))
			{
				ComponentData<TComponent>* comp = static_cast<ComponentData<TComponent>*>(comp_data.get());
				comp->set_new_component_value(component);
				return comp;
			}
		}
		return add_component(component);
	}

	template <typename TComponent>
	ComponentData<TComponent>* replace_component(const TComponent& component)
	{
		for (const auto& comp_data : component_datas)
		{
			if (comp_data->get_type() == typeid(TComponent))
			{
				ComponentData<TComponent>* comp = static_cast<ComponentData<TComponent>*>(comp_data.get());
				comp->set_new_component_value(component);
				return comp;
			}
		}
		return nullptr;
	}

	template <typename TComponent>
	ComponentData<TComponent>* try_get_component()
	{
		for (const auto& comp_data : component_datas)
		{
			if (comp_data->get_type() == typeid(TComponent))
			{
				ComponentData<TComponent>* comp = static_cast<ComponentData<TComponent>*>(comp_data.get());
				return comp;
			}
		}
		return nullptr;
	}




	template <typename TComponent>
	bool replace_component_data(TComponent&& a_component)
	{
		//static_assert(std::is_standard_layout<std::decay_t<TComponent>>::value, "TComponent must be a standard-layout type (e.g no inheritance)");
		//static_assert(std::is_trivial<std::decay_t<TComponent>>::value, "TComponent must be trivial (e.g. no smart pointers)");
		std::type_index t_type = typeid(std::decay_t<TComponent>);
		bool error = false;
		for (size_t i = 0; i < component_types.size(); i++) // may want to consider to sort these in order of each other.
		{
			if (t_type == component_types[i])
			{
				for (size_t i = 0; i < component_datas.size(); i++)
				{
					if (component_datas[i]->get_type() == t_type)
					{
						ComponentData<std::decay_t<TComponent>>& component_data = static_cast<ComponentData<std::decay_t<TComponent>>&>(*component_datas[i]);
						std::decay_t<TComponent> component = std::forward<TComponent>(a_component);
						component_data.set_new_component_value(component);
						return true;
					}
				}
				error = true;
				break;
			}
		}
		if (error)
		{
			PRINTLN("MISSMATCH: scene_object has component_type info of component for which there's no comopnent_data for");
			// perhaps remove the component_type ?
		}
		return false;
	}

	template <typename TComopnentData, typename TComponent>
	bool add_or_replace_component_data(TComponent&& a_component)
	{
		if (!replace_component_data<std::decay_t<TComponent>>(std::forward<TComponent>(a_component)))
		{
			if (!add_component_data<TComopnentData>(std::forward<TComponent>(a_component)))
			{
				PRINTLN("ERROR: somehow component could not be replaced, and not added");
				return false;
			}
			return true;
		}
		return false;
	}


	template<typename TComponent>
	bool get_component(TComponent*& a_component)
	{
		//static_assert(std::is_standard_layout<std::decay_t<TComponent>>::value, "TComponent must be a standard-layout type (e.g no inheritance)");
		//static_assert(std::is_trivial<std::decay_t<TComponent>>::value, "TComponent must be trivial (e.g. no smart pointers)");
		std::type_index t_type = typeid(std::decay_t<TComponent>);
		bool error = false;
		for (size_t i = 0; i < component_types.size(); i++) // may want to consider to sort these in order of each other.
		{
			if (t_type == component_types[i])
			{
				for (size_t i = 0; i < component_datas.size(); i++)
				{
					if (component_datas[i]->get_type() == t_type)
					{
						ComponentData<std::decay_t<TComponent>>& component_data = static_cast<ComponentData<std::decay_t<TComponent>>&>(*component_datas[i]);
						a_component = &component_data.get_component();
						if (a_component == nullptr) return false;
						return true;
					}
				}
				error = true;
				break;
			}
		}
		if (error)
		{
			PRINTLN("MISSMATCH: scene_object has component_type info of component for which there's no comopnent_data for");
			// perhaps remove the component_type ?
		}
		return false;
	}

	template<typename TComponent>
	TComponent* get_component()
	{
		//static_assert(std::is_standard_layout<std::decay_t<TComponent>>::value, "TComponent must be a standard-layout type (e.g no inheritance)");
		//static_assert(std::is_trivial<std::decay_t<TComponent>>::value, "TComponent must be trivial (e.g. no smart pointers)");
		std::type_index t_type = typeid(std::decay_t<TComponent>);
		bool error = false;
		for (size_t i = 0; i < component_types.size(); i++) // may want to consider to sort these in order of each other.
		{
			if (t_type == component_types[i])
			{
				for (size_t i = 0; i < component_datas.size(); i++)
				{
					if (component_datas[i]->get_type() == t_type)
					{
						ComponentData<std::decay_t<TComponent>>& component_data = static_cast<ComponentData<std::decay_t<TComponent>>&>(*component_datas[i]);
						return &component_data.get_component();
					}
				}
				error = true;
				break;
			}
		}
		if (error)
		{
			PRINTLN("MISSMATCH: scene_object has component_type info of component for which there's no comopnent_data for");
			// perhaps remove the component_type ?
		}
		return nullptr;
	}

	bool remove_component_data(size_t a_index)
	{
		if (a_index < component_datas.size())
		{
			auto& component_data = component_datas[a_index];
			component_data->destroy_component();
			component_datas.erase(component_datas.begin() + a_index);
			return true;
		}
		return false;
	}

	template<typename TComponent>
	bool remove_component_data()
	{

		std::type_index t_type = typeid(TComponent);
		size_t index = 0;
		bool found_index = false;
		bool found_component = false;
		for (size_t i = 0; i < component_types.size(); i++) // may want to consider to sort these in order of each other.
		{
			if (t_type == component_types[i])
			{
				index = i;
				found_index = true;
				for (size_t i = 0; i < component_datas.size(); i++)
				{
					if (component_datas[i]->get_type() == t_type)
					{
						component_datas.erase(component_datas.begin() + i);
						found_component = true;
						break;
					}
				}
				break;
			}
		}
		if (found_component != found_index)
		{
			PRINTLN("MISSMATCH: scene_object has component_type info of component for which there's no comopnent_data for");
			// perhaps remove the component_type ?
		}
		if (found_index)
		{
			component_types.erase(component_types.begin() + index);
		}
		return found_component;
	}

	entt::handle start_runtime(std::weak_ptr<Scene> a_scene);
	void get_entity_references();
	//void draw_components();

	size_t num_children() const { return children.size(); };
	std::string get_name() 
	{
		if (EntityComponent* name_comp; get_component<EntityComponent>(name_comp))
		{
			name = name_comp->hashed_name.string;
		}
		return name;
	};

	std::vector<std::weak_ptr<SceneObject>> get_children() { return children; };

	const std::vector<std::unique_ptr<ComponentDataBase>>& get_component_datas() const
	{ return component_datas; }
	bool is_runtime() const { return runtime; }
	bool check_valid(size_t a_index);
};

