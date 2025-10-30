#pragma once
#include <vector>
#include "Renderable.h"
class Scene
{
public:
	//FrameBuffer* aFrameBuffer later
	Scene();
	virtual void AddRenderable(Renderable* aRenderable);
	virtual void RemoveRenderable(Renderable* aRenderable);
	std::vector<Renderable*>& GetRenderables();
	bool isSorted = false;
	
	//FrameBuffer* frameBuffer;

private:
	std::vector<Renderable*> renderables;
	

};

