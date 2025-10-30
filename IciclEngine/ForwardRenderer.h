#pragma once
#include "Renderer.h"

class ForwardRenderer : Renderer
{
public:
	ForwardRenderer();
	~ForwardRenderer();
	void RenderFrame(Scene& aScene) override;
	void Init() override;

	void AddRenderPass(Shader& aShader);
protected:
	void SortRenderPasses();
	bool isSceneSorted = false;
};

