#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderable.h"

class GameObject
{
public:
	 glm::vec3 worldPosition;
	 glm::vec3 rotation;
	 glm::vec3 scale;
	 Renderable* renderable;
	 GameObject(std::string aLabel, glm::vec3 aPosition, glm::vec3 aRotation, glm::vec3 aScale);
	 GameObject(std::string aLabel, glm::vec3 aPosition, glm::vec3 aRotation, glm::vec3 aScale, Renderable* aRenderable);
	 ~GameObject(){};

	 virtual void Update(float dt);
	 void AddRenderable(Renderable* aRenderable);
	 std::string label;
};

