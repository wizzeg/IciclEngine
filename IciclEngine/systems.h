#pragma once

#include <engine/utilities/macros.h>
#include <memory>

class Scene;

enum SystemTypes
{
	unassigned,
	single_thread,
};


struct System
{
	SystemTypes type = unassigned;
	virtual void init() {}
	virtual void execute(std::weak_ptr<Scene> a_scene) { PRINTLN("no system implementation"); };
};

struct UIDrawerSystem : System
{
	void init() override { type = single_thread; }
	void execute(std::weak_ptr<Scene> a_scene) override;
};

struct MeshRendererSystem : System
{
	void init() override { type = single_thread; }
	void SetRenderer() {};
	void execute(std::weak_ptr<Scene> a_scene) override;
	void render();
};