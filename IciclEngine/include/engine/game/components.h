#pragma once
#include <string>
#include <typeindex>
#include <unordered_map>
#include <functional> 
#include <algorithm>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
// #include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <engine/renderer/render_info.h>
#include <engine/resources/obj_parser.h>
#include <engine/utilities/entt_modified.h>

struct NameComponent
{
	std::string name;
};

struct WorldPositionComponent
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 rotation_euler_do_not_use = glm::vec3(0.0f);
    glm::quat rotation_quat = glm::quat(1.0f, 0.f, 0.f, 0.f);
    bool get_euler_angles = true;
    bool overide_quaternion = false;
    glm::mat4 model_matrix = glm::mat4(1);
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
    bool loaded;
    hashed_string_64 hashed_path;
};

struct TextureComponent
{
    bool loaded;
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
    glm::mat4 projection_matrix = glm::mat4(1);

    glm::mat4 view_matrix = glm::mat4(1);

    hashed_string_64 frame_buffer_target = hashed_string_64("main_camera_buffer");
    glm::vec3 target_location = glm::vec3(0);
    entt::entity target_entity = entt::null;

    uint16_t render_priority = 5000;
    float fied_of_view = 70.f;
    float near_plane = 300.f;
    float far_plane = 0.1f;
    float aspect_ratio = 720.f / 480.f;
    bool orbit_camera = false;
    bool wants_to_render = true;
    bool clear_color_buffer = true;
    bool clear_depth_buffer = true;
};


struct FrameBufferComponent
{
    // perhaps to make a new frame buffer
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