#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderable.h"

class GameObject
{
public:
	 glm::vec3 worldPosition;
	 Renderable* renderable;

	 GameObject(glm::vec3 position, Renderable* renderable) : worldPosition(position), renderable(renderable) {  };
};

