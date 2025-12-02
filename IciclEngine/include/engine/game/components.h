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
    hashed_string_64 hashed_path;
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

struct CameraComponent
{
    bool render_from;
    glm::mat4 projection_matrix;
    glm::mat4 view_matrix;
    hashed_string_64 buffer_target;
};

struct ParentComponent
{
    entt::entity parent = entt::null;
};

struct SiblingComponent
{
    entt::entity next_sibling = entt::null;
    entt::entity previous_sibling = entt::null;
};

struct ChildComponent
{
    entt::entity child = entt::null;
};

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    entt::entity next_sibling = entt::null;
    entt::entity previous_sibling = entt::null;
    entt::entity child = entt::null;
    uint8_t depth;
};

// Perhaps use depth comopnents
struct HierarchyDepth // perhaps only have one that says the level...however, views get all of them, no better than looking for ones without a parent
{                     // However, after a sort based on depth, parents are computed, and likely in cache, then can quickly get their parent component.
    uint8_t depth;
};

struct HierarchyDepthOne // first child level - this gives perfect view groups, and I have the parents matrices ready to aggregate... I think this is most efficient
{

};
struct HierarchyDepthTwo // second child level
{

};

//etc

struct HierarchyDepthDeep // past predefined ... Do a slower method for the exceptionally deep hierarchys
{

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