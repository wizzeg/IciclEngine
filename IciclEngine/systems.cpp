#include "systems.h"
#include <engine/editor/scene.h>
#include <engine/utilities/entt_modified.h>
#include <engine/game/components.h>

void UIDrawerSystem::execute(std::weak_ptr<Scene> a_scene)
{
	if (auto scene = a_scene.lock())
	{
		//scene->draw_imgui();
	}
}

void MeshRendererSystem::execute(std::weak_ptr<Scene> a_scene)
{
	if (auto scene = a_scene.lock())
	{
		auto& registry = scene->get_registry();
		for (auto [entity, worldpos, renderable] : registry.view<TransformDynamicComponent, RenderableComponent>().each())
		{
			
		}
	}
}

void MeshRendererSystem::render()
{

}