#pragma once

#include <vector>
#include "RenderPass.h"
#include "Scene.h"

class Renderer
{
public:
	Renderer();
	~Renderer();
	virtual void Init();
	virtual void BeginFrame(Scene& aScene);
	virtual void RenderFrame(Scene& aScene);
	virtual void EndFrame(Scene& aScene);

protected:
	std::vector<RenderPass*> renderPasses;
};