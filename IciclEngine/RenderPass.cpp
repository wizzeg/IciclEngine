#include "RenderPass.h"

RenderPass::RenderPass() {}
RenderPass::~RenderPass() {}

void RenderPass::Render(Scene& aScene)
{
	std::vector<Renderable*> renderables = aScene.GetRenderables();
	if (renderables.empty()) return;
	for (Renderable*& renderable : renderables)
	{
		renderable->Render();
	}
}