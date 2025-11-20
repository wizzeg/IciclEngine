#include <engine/editor/scene_object.h>
#include <engine/editor/scene.h>
#include <engine/utilities/macros.h>


SceneObject::SceneObject(const std::string a_name, std::weak_ptr<Scene> a_scene)
	: name(a_name), scene(a_scene)
{
	component_datas.emplace_back(std::make_unique<NameComponentData>(NameComponent{ a_name }));
	component_types.emplace_back(typeid(NameComponent));
}

SceneObject::SceneObject(const std::string a_name, std::weak_ptr<SceneObject> a_parent, std::weak_ptr<Scene> a_scene)
	: name(a_name), parent(a_parent), scene(a_scene)
{
	PRINTLN("DO NOT USE THIS SCENE OBJECT CONSTRUCTOR YET");
	component_datas.emplace_back(std::make_unique<ComponentData<NameComponentData>>(NameComponent{ a_name }));
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

entt::handle SceneObject::get_entity_handle() const
{
	return entity_handle;
}

entt::handle SceneObject::to_runtime(std::weak_ptr<Scene> a_scene)
{
	runtime = true;
	PRINTLN("runtime started");
	if (auto scene = a_scene.lock())
	{
		entity_handle = scene->create_entity(name);
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			component_datas[i]->to_runtime(entity_handle);
		}
	}
	return entity_handle;
}

void SceneObject::scene_ended()
{
	runtime = false;
}