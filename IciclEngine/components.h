#pragma once
#include <string>
#include <typeindex>
#include <unordered_map>
#include <functional> 
#include <algorithm>
#include <utility>
#include <glm/glm.hpp>
#include "render_info.h"

struct NameComponent
{
	std::string name; // string not ideal, but I rather not have to deal with char limitations etc, especially with paths
};

struct WorldPositionComponent
{
    glm::vec3 position;
};

struct RenderableComponent
{
    MeshHandle meshID;
    MaterialHandle mateiralID;
};

//static_assert(std::is_trivially_copyable<WorldPositionComponent>::value, "MyPOD is not a POD type");

// perhaps gonna use this after all, with some form of reflecaiton maybe, to still let users define how to draw, but hide the logic of choosing which component to modify
class StaticComponentUIDrawer // not used
{
public:
    // add every drawable component here
    inline static std::unordered_map<std::type_index, std::function<void()>> UI_drawers{  // input paramatern needs to be T... hm
        {typeid(NameComponent), []() { /* ui draw ... but I need the component data too... */ },
        }
    };
};