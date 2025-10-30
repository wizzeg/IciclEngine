#pragma once
#include "Renderer.h"

class EngineContext
{
public:
	// add DebugConsole* aDebugConsole
	EngineContext(Renderer* aRenderer);
	EngineContext(Renderer* aRenderer, Scene* aScene);
	~EngineContext();
	void DrawScene();
	void DrawConsole() {};
	void SetScene(Scene* aScene);
	Scene* GetScene();

protected:
	Scene* scene;
	Renderer* renderer;
};

