#pragma once
#include <string>
#include <typeindex>
#include <unordered_map>
#include <functional> 
#include <algorithm>
#include <utility>
#include <glm/glm.hpp>

struct NameComponent
{
	std::string name;
};

struct WorldPositionComponent
{
    glm::vec3 position;
};

class StaticComponentUIDrawer
{
public:
    // add every drawable component here
    inline static std::unordered_map<std::type_index, std::function<void()>> UI_drawers{  // input paramatern needs to be T... hm
        {typeid(NameComponent), []() { /* ui draw ... but I need the component data too... */ },
        }
    };
};