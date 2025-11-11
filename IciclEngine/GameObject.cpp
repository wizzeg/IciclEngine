#include "GameObject.h"
#include <print>

GameObject::GameObject(std::string aLabel, glm::vec3 aPosition, glm::vec3 aRotation, glm::vec3 aScale)
	: label(aLabel), worldPosition(aPosition), rotation(aRotation), scale(aScale)
{
}

GameObject::GameObject(std::string aLabel, glm::vec3 aPosition, glm::vec3 aRotation, glm::vec3 aScale, Renderable* aRenderable)
	: label(aLabel), worldPosition(aPosition), rotation(aRotation), scale(aScale), renderable(aRenderable)
{
	renderable->position = &worldPosition;
	renderable->rotation = &rotation;
	renderable->scale = &scale;
}

void GameObject::Update(float dt)
{
	//std::println("updating {} ", rotation.y);
	//rotation.y += dt * 50;
}

void GameObject::AddRenderable(Renderable* aRenderable)
{
	renderable = aRenderable;
	renderable->position = &worldPosition;
	renderable->rotation = &rotation;
	renderable->scale = &scale;
}