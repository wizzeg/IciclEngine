#pragma once
#include "ForwardRenderer.h"
#include <algorithm>
#include "ForwardRenderPass.h"
#include <print>

ForwardRenderer::ForwardRenderer() {}

ForwardRenderer::~ForwardRenderer() {}

void ForwardRenderer::RenderFrame(Scene& aScene)
{
	std::vector<Renderable*>& renderables = aScene.GetRenderables();
	if (!(aScene.isSorted))
	{
		std::println("Sorting renderables");
		//std::sort(renderables.begin(), renderables.end());
		std::sort(renderables.begin(), renderables.end(), [](Renderable* a, Renderable* b) {
			return *a < *b;
			});
		aScene.isSorted = true;
		for (size_t i = 0; i < renderables.size(); i++)
		{
			std::println("Renderable program: {}", renderables[i]->GetShaderProgram());
		}
	}

	unsigned int currentShader = 0;
	unsigned int currentRenderable = 0;
	//std::println("number of renderpasses: {}", renderPasses.size());
	for(auto pass : renderPasses)
	{
		auto forwardPass = dynamic_cast<ForwardRenderPass*>(pass);
		if (forwardPass != nullptr)
		{

			currentRenderable = forwardPass->RenderFromIndex(aScene, currentRenderable);

			//unsigned int program = forwardPass->GetShaderProgram();
			//glUseProgram(program);
			//for (currentRenderable; currentRenderable < renderables.size(); currentRenderable++)
			//{
			//	if (renderables[currentRenderable]->shaderProgram == program)
			//	{
			//		std::println("drawing mesh with shader program {}", program);
			//		renderables[currentRenderable]->DrawMesh();
			//	}
			//	else if (renderables[currentRenderable]->shaderProgram > program)
			//	{
			//		std::println("shader program finished {}", program);
			//		break;
			//	}
			//	else
			//	{
			//		std::println("renderable does not have an assigned render program (or incorrect sorting)");
			//	}
			//}
		}
		else
		{
			std::println("Failed casting!!!!!!!!");
		}
	}

}

void ForwardRenderer::Init() { }

void ForwardRenderer::AddRenderPass(Shader& aShader)
{
	std::println("Got new shaderprogram: {}", aShader.GetShaderProgram());
	if (aShader.GetShaderProgram() == 0) return;
	for (auto pass : renderPasses)
	{
		auto forwardPass = dynamic_cast<ForwardRenderPass*>(pass);
		if (forwardPass != nullptr)
		{
			if (forwardPass->GetShaderProgram() == aShader.GetShaderProgram())
			{
				std::println("identical shader program already exists");
				return;
			}
		}
	}
	ForwardRenderPass* pass = new ForwardRenderPass(&aShader);
	renderPasses.push_back(pass);
	SortRenderPasses();

}

void ForwardRenderer::SortRenderPasses()
{
	std::sort(renderPasses.begin(), renderPasses.end(), [](RenderPass* a, RenderPass* b) {
		auto fa = dynamic_cast<ForwardRenderPass*>(a);
		auto fb = dynamic_cast<ForwardRenderPass*>(b);
		if (fa && fb) return fa->GetShaderProgram() < fb->GetShaderProgram();
		else if (fa) return true;
		else if (fb) return false;
		else
		{
			std::println("could not compare renderpass");
			return false;
		}
	});
	for (size_t i = 0; i < renderPasses.size(); i++)
	{
		auto fr = dynamic_cast<ForwardRenderPass*>(renderPasses[i]);
		std::println("RenderProgram number: {}", fr->GetShaderProgram());
	}
}