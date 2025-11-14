//#include "entity.h"
//
//
//Entity::Entity() { }
//Entity::Entity(std::weak_ptr<Scene> a_scene)
//{
//	scene = a_scene;
//	if (auto shared = scene.lock())
//	{
//		auto& registry = shared->get_registry();
//		auto entt = registry.create();
//		entity_handle = entt::handle{registry, entt};
//	}
//}
//Entity::Entity(entt::entity a_entity, std::weak_ptr<Scene> a_scene)
//{
//	scene = a_scene;
//	if (auto shared = scene.lock())
//	{
//		entity_handle = entt::handle{ shared->get_registry(), a_entity };
//	}
//}
//Entity::~Entity() {}