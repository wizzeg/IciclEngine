#include "EngineContext.h"

EngineContext::EngineContext(Renderer* aRenderer) : renderer(aRenderer)
{
	renderer->Init();
	scene = nullptr;
}

EngineContext::EngineContext(Renderer* aRenderer, Scene* aScene) : renderer(aRenderer), scene(aScene)
{
	renderer->Init();
}

EngineContext::~EngineContext()
{
	scene = nullptr;
	renderer = nullptr;
}

void EngineContext::DrawScene()
{
	renderer->BeginFrame(*scene);
	renderer->RenderFrame(*scene);
	renderer->EndFrame(*scene);
}

void EngineContext::SetScene(Scene* aScene) { scene = aScene; }

Scene* EngineContext::GetScene() { return scene; }