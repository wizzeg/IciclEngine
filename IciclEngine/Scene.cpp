#include "Scene.h"
#include <print>

Scene::Scene() {};

void Scene::AddRenderable(Renderable* aRenderable)
{
	renderables.push_back(aRenderable);
	isSorted = false;
}
void Scene::RemoveRenderable(Renderable* aRenderable)
{
	//std::vector<Renderable*>::iterator
	for (auto it = renderables.begin(); it != renderables.end();)
	{
		if (*it == aRenderable)
		{
			std::println("Removed a renderable");
			renderables.erase(it);
			break;
		}
	}
}

std::vector<Renderable*>& Scene::GetRenderables()
{
	return renderables;
}
