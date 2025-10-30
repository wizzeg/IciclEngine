#pragma once

#include "Scene.h"

class RenderPass
{
public:
	RenderPass();
	~RenderPass();
	virtual void Render(Scene& aScene);
};

