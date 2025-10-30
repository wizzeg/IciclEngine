#include "ForwardRenderPass.h"
#include <print>

ForwardRenderPass::ForwardRenderPass(Shader* aShader) : shader(aShader)
{ 
	shaderProgram = shader->GetShaderProgram();
}

ForwardRenderPass::~ForwardRenderPass()
{
}

void ForwardRenderPass::Render(Scene& aScene)
{
	std::println("Do not use Render with ForwardRenderer");
	//if (&shader == nullptr)
	//{
	//	std::println("Shader in forward shaderpass not set");
	//}
	//shader->Use();
	//auto renderables = aScene.GetRenderables();
	//for (auto& renderable : renderables)
	//{
	//	renderable->Render();
	//}
	//shader->Stop();
}

unsigned int ForwardRenderPass::RenderFromIndex(Scene& aScene, unsigned int index)
{
	if (&shader == nullptr)
	{
		std::println("Shader in forward shaderpass not set");
	}
	shader->Use();
	auto renderables = aScene.GetRenderables();
	for (;index < renderables.size(); index++)
	{
		if (renderables[index]->shaderProgram == shaderProgram)
		{
			//std::println("drawing shading program {}", shaderProgram);
			renderables[index]->DrawMesh();
		}
		else if (renderables[index]->shaderProgram > shaderProgram)
		{
			//std::println("shader program finished {}", shaderProgram);
			break;
		}
		else
		{
			std::println("renderable does not have an assigned render program (or incorrect sorting) {}", renderables[index]->GetShaderProgram());
		}
	}
	return index;
}