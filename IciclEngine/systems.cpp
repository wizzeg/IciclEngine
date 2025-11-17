#include "systems.h"
#include "scene.h"
#include <entt/entt.hpp>
#include "components.h"

void UIDrawerSystem::execute(std::weak_ptr<Scene> a_scene)
{
	if (auto scene = a_scene.lock())
	{
		scene->draw_imgui();
	}
}

void MeshRendererSystem::execute(std::weak_ptr<Scene> a_scene)
{
	if (auto scene = a_scene.lock())
	{
		auto& registry = scene->get_registry();
		for (auto [entity, worldpos, renderable] : registry.view<WorldPositionComponent, RenderableComponent>().each())
		{
			
		}
	}
}

void MeshRendererSystem::render()
{

}