#include "Renderer.h"
#include <glad/glad.h>

Renderer::Renderer()
{

}

Renderer::~Renderer() {}

void Renderer::BeginFrame(Scene& aScene)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.1f, 0.3f, 0.2f, 1.0f);
}
void Renderer::RenderFrame(Scene& aScene)
{
	if (renderPasses.empty()) return;

	//std::vector<Renderable*> renderables = aScene.GetRenderables();
	for (RenderPass*& renderPass : renderPasses)
	{
		renderPass->Render(aScene);
	}

	//for (Renderable*& renderable : renderables)
	//{
	//	renderable->Render();
	//}
}

void Renderer::EndFrame(Scene& aScene)
{
	glBindVertexArray(0);
	glUseProgram(0);
}

void Renderer::Init()
{
	RenderPass* renderpass = new RenderPass();
	renderPasses.push_back(renderpass);
}