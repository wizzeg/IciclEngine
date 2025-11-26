#pragma once
#include <string>
#include <typeindex>
#include <unordered_map>
#include <functional> 
#include <algorithm>
#include <utility>
#include <glm/glm.hpp>
#include <engine/renderer/render_info.h>
#include <engine/resources/obj_parser.h>

struct NameComponent
{
	std::string name;
};

struct WorldPositionComponent
{
    glm::vec3 position;
};

struct MeshLoaderComponent
{
    std::string path;
};

struct MaterialLoaderComponent
{
    std::string path;
};

struct MeshComponent
{
    uint32_t id;
    std::string path;
    entt::hashed_string hashed_path;
};

struct MaterialComponent
{
    uint32_t id;
    std::string path;
};

struct RenderableComponent
{
    uint32_t mesh_id;
    uint32_t mateiral_id;
};

// perhaps gonna use this after all, with some form of reflecaiton maybe, to still let users define how to draw, but hide the logic of choosing which component to modify
// this no longer needed
class StaticComponentUIDrawer // not used
{
public:
    // add every drawable component here
    inline static std::unordered_map<std::type_index, std::function<void()>> UI_drawers{  // input paramatern needs to be T... hm
        {typeid(NameComponent), []() { /* ui draw ... but I need the component data too... */ },
        }
    };
};