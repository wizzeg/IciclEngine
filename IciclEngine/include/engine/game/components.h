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
#include <engine/resources/data_structs.h>

/// <summary>
/// 0 : int
/// 1 : float
/// 3 : ivec1
/// 4 : vec2
/// 5 : vec3
/// 6 : vec4
/// 7 : mat3
/// 8 : mat4
/// 
/// </summary>

struct NameComponent
{
    hashed_string_64 hashed_name;
	//std::string name;
    EntityReference entity;
};

struct TransformDynamicStaticComponent
{
    // to convert Dynamic to static ... Basically check for this, if it has Dynamic, copy model matrix and add TransformStatic with model matrix
    // or alternatively a tag that just removes the Transform component
    bool dynamic = false;
};

struct TransformStaticComponent
{
    glm::mat4 model_matrix = glm::mat4(1);
};

struct TransformDynamicComponent
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 rotation_euler_do_not_use = glm::vec3(0.0f);
    glm::quat rotation_quat = glm::quat(1.0f, 0.f, 0.f, 0.f);
    bool get_euler_angles = false;
    bool overide_quaternion = false;
    glm::mat4 model_matrix = glm::mat4(1);
};

struct TransformComponent
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 rotation_euler_do_not_use = glm::vec3(0.0f);
    glm::quat rotation_quat = glm::quat(1.0f, 0.f, 0.f, 0.f);
    bool get_euler_angles = false;
    bool overide_quaternion = false;
};

struct TransformNoRotationComponent
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct TransformRotationComponent
{
    glm::vec3 rotation_euler_do_not_use = glm::vec3(0.0f);
    glm::quat rotation_quat = glm::quat(1.0f, 0.f, 0.f, 0.f);
    bool get_euler_angles = false;
    bool overide_quaternion = false;
};

struct ModelMatrixComponent // or use this, and remove from TransformDynamicComponent, then this is basically a static transform
{
    glm::mat4 model_matrix = glm::mat4(1);
    void compute_model_matrix(glm::vec3 a_pos, glm::vec3 a_scale = glm::vec3(1), glm::quat a_rot = glm::vec3(0))
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, a_pos);
        model *= glm::mat4_cast(a_rot);
        model = glm::scale(model, a_scale);
        //model_matrix = model;
    }
    void compute_model_matrix(glm::vec3 a_pos, glm::vec3 a_scale = glm::vec3(1), glm::vec3 a_rot = glm::vec3(0))
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, a_pos);
        model = glm::rotate(model, glm::radians(a_rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(a_rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(a_rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, a_scale);
        //model_matrix = model;
    }
};

//struct TransformStaticComponent
//{
//    glm::mat4 model_matrix = glm::mat4(1.f);
//    void compute_model_matrix(glm::vec3 a_pos, glm::vec3 a_scale = glm::vec3(1), glm::vec3 a_rot = glm::vec3(0))
//    {
//        glm::mat4 model = glm::mat4(1.0f);
//        model = glm::scale(model, a_scale);
//        model = glm::rotate(model, glm::radians(a_rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
//        model = glm::rotate(model, glm::radians(a_rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
//        model = glm::rotate(model, glm::radians(a_rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
//        model = glm::translate(model, a_pos);
//        model_matrix = model;
//    }
//};
//
//struct DeltaMovement // later for optimization, use these + static.
//{
//    glm::vec3 delta_position;
//};
//struct DeltaScale
//{
//    glm::vec3 delta_scale;
//};
//struct DeltaRotation
//{
//    glm::vec3 rotation_euler = glm::vec3(0.0f);
//    glm::quat rotation_quat = glm::quat(1.0f, 0.f, 0.f, 0.f);
//};

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
    hashed_string_64 hashed_path;
    bool instance;
    bool mipmap = true;
    bool load;
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
    //entt::entity target_entity = entt::null;
    EntityReference target_entity = EntityReference(entt::null, 0);

    uint16_t render_priority = 5000;
    float field_of_view = 70.f;
    float near_plane = 300.f;
    float far_plane = 0.1f;
    float aspect_ratio = 720.f / 480.f;
    bool orbit_camera = false;
    bool wants_to_render = true;
    bool clear_color_buffer = true;
    bool clear_depth_buffer = true;
};

struct ShadingLightComponent
{
    glm::vec4 light_ambient = glm::vec4(0.2, 0.2, 0.2, 1);
    glm::vec4 light_diffuse = glm::vec4(1, 0.9, 0.8, 1);
    glm::vec4 light_specular = glm::vec4(0.9, 0.8, 0.7, 1);
    glm::vec3 light_attenuation = glm::vec3(1, 0.1, 0.001);
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
    EntityReference parent;
    EntityReference next_sibling;
    EntityReference previous_sibling;
    EntityReference child;
    //entt::entity parent = entt::null;
    //entt::entity next_sibling = entt::null;
    //entt::entity previous_sibling = entt::null;
    //entt::entity child = entt::null;
    //uint8_t depth;
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


struct TestComponent
{
    uint64_t test = 0;
};

struct RenderComponent
{
    hashed_string_64 mesh;
    hashed_string_64 material;
    bool instance;
    bool mipmap;
    bool load;
};

struct PointLightComponent
{
    glm::vec3 color = glm::vec3(1);
    glm::vec3 attenuation = glm::vec3(0.75f, 0.1f, 0.01f);
    float intensity = 0.5f;
    bool shadow_map = false;
};

struct DirectionalLightComponent // I don't know what else I need for these
{
    glm::vec3 color;
    glm::vec3 rotation_euler_do_not_use = glm::vec3(0.0f);
    glm::quat rotation_quat = glm::quat(1.0f, 0.f, 0.f, 0.f);
    bool get_euler_angles = false;
    bool overide_quaternion = false;
    bool shadow_map = true;
    float intensity;
};

struct MaterialFloatComponent
{
    hashed_string_64 material = "";
    std::string location = "";
    float value = 0;
    float prev_value = 0;
    bool set = false;
    bool continous_update = true;
};

struct MaterialIntComponent
{
    hashed_string_64 material;
    std::string location;
    uint32_t value;
    bool set;
};

struct MaterialUniformComponent
{
    uint8_t type = 0;
    hashed_string_64 material = "";
    std::string location = "";
    UniformValue value = (int)0;
    bool set = false;
};

struct TestReferenceComponent
{
    EntityReference reference;
};