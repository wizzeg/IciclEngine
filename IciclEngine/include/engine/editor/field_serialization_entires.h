#pragma once // DO NOT INCLUDE THIS FILE IN ANY OTHER FILE
#include <engine/editor/field_serialization_macro.h>
#include <glm/glm.hpp>
#include <engine/utilities/entt_modified.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <engine/editor/scene.h>
#include <string>
#include <engine/game/components.h>
#include <engine/resources/data_structs.h>
#include <engine/utilities/macros.h>
#include <any>
#include <variant>
#include <type_traits> 

//using UniformValue = std::variant<bool, int, float, double, glm::vec3, glm::vec4, glm::quat, glm::mat4, glm::ivec1, std::string>;


// this one is actually not needed, Texture is basically a string...
//REGISTER_TYPE_SERIALIZER(UniformTexture,
//    {
//        j = json::object();
//        j["file_path"] = value.file_path;
//        j["location"] = value.uniform_name;
//    },
//    {
//        // should do check that it is object etc
//        value.file_path = j["file_path"].get<std::string>();
//        value.uniform_name = j["location"].get<std::string>();
//    })

REGISTER_TYPE_SERIALIZER(EntityReference,
    {
        j = value.scene_object;
// Don't serialize entity
    },
    {
        value.scene_object = j.get<uint32_t>();
        value.entity = entt::null;
    }
)

REGISTER_TYPE_SERIALIZER(glm::ivec1,
    {
        j = json::array();
        const int* v = static_cast<const int*>(ptr);
        j.push_back(v[0]);
    },
    {
        glm::ivec1 & v = *static_cast<glm::ivec1*>(ptr);
        if (j.is_array() && j.size() == 1) {
            v.x = j[0].get<int>();
        }
    }
);

//REGISTER_TYPE_SERIALIZER(glm::vec1,
//    {
//        j = json::array();
//        const float* v = static_cast<const float*>(ptr);
//        PRINTLN("writing val: {}", v[0]);
//        j.push_back(v[0]);
//    },
//    {
//        glm::vec1 & v = *static_cast<glm::vec1*>(ptr);
//        if (j.is_array() && j.size() == 1) {
//            v.x = j[0].get<float>();
//        }
//    }
//);

REGISTER_TYPE_SERIALIZER(glm::vec1,
    {
        j = json::array();
        const glm::vec1& v = *static_cast<const glm::vec1*>(ptr);
        j.push_back(v.x);
    },
    {
        glm::vec1 & v = *static_cast<glm::vec1*>(ptr);
        if (j.is_array() && j.size() == 1) {
            v.x = j[0].get<float>();
        }
    }
);

REGISTER_TYPE_SERIALIZER(glm::vec3,
    { j = json::array({value.x, value.y, value.z}); },
    {
        if (j.is_array() && j.size() == 3) {
            value.x = j[0].get<float>();
            value.y = j[1].get<float>();
            value.z = j[2].get<float>();
        }
    }
)

// glm::vec4
REGISTER_TYPE_SERIALIZER(glm::vec4,
    { j = json::array({value.x, value.y, value.z, value.w}); },
    {
        if (j.is_array() && j.size() == 4) {
            value.x = j[0].get<float>();
            value.y = j[1].get<float>();
            value.z = j[2].get<float>();
            value.w = j[3].get<float>();
        }
    }
)

// glm::vec2
REGISTER_TYPE_SERIALIZER(glm::vec2,
    { j = json::array({value.x, value.y}); },
    {
        if (j.is_array() && j.size() == 2) {
            value.x = j[0].get<float>();
            value.y = j[1].get<float>();
        }
    }
)

// glm::quat
REGISTER_TYPE_SERIALIZER(glm::quat,
    { j = json::array({value.x, value.y, value.z, value.w}); },
    {
        if (j.is_array() && j.size() == 4) {
            value.x = j[0].get<float>();
            value.y = j[1].get<float>();
            value.z = j[2].get<float>();
            value.w = j[3].get<float>();
        }
    }
)

// glm::mat4
REGISTER_TYPE_SERIALIZER(glm::mat4,
    {
        json j_arr = json::array();
        const float* val_ptr = glm::value_ptr(value);
        int idx = 0;
        //for (int i = 0; i < 4; i++) {
        //    for (int k = 0; k < 4; k++) {
        //        j.push_back((val_ptr[idx++]));
        //    }
        //}
        for (int i = 0; i < 16; i++) j_arr.push_back(val_ptr[i]);
        j = j_arr;
    },
    {
        if (j.is_array() && j.size() == 16) {
            int idx = 0;
            for (int i = 0; i < 4; i++) {
                for (int k = 0; k < 4; k++) {
                    value[i][k] = j[idx++].get<float>();
                }
            }
        }
    }
)

// std::string
REGISTER_TYPE_SERIALIZER(std::string,
    { j = value; },
    { value = j.get<std::string>(); }
    )

// entt::entity
REGISTER_TYPE_SERIALIZER(entt::entity,
    { j = static_cast<uint32_t>(entt::null); },
    //{ value = static_cast<entt::entity>(j.get<uint32_t>()); }
    { value = entt::null; }
)

    // hashed_string_64
REGISTER_TYPE_SERIALIZER(hashed_string_64,
    { j = value.string; },
    { value = hashed_string_64(j.get<std::string>().c_str()); }
)

// Primitives
REGISTER_TYPE_SERIALIZER(int,
    { j = value; },
    { value = j.get<int>(); }
)

REGISTER_TYPE_SERIALIZER(int64_t,
    { j = value; },
    { value = j.get<int64_t>(); }
)

REGISTER_TYPE_SERIALIZER(uint64_t,
    { j = value; },
    { value = j.get<uint64_t>(); }
)

REGISTER_TYPE_SERIALIZER(uint32_t,
    { j = value; },
    { value = j.get<uint32_t>(); }
)

REGISTER_TYPE_SERIALIZER(uint16_t,
    { j = value; },
    { value = j.get<uint16_t>(); }
)

REGISTER_TYPE_SERIALIZER(uint8_t,
    { j = value; },
    { value = j.get<uint8_t>(); }
)

REGISTER_TYPE_SERIALIZER(float,
    { j = value; },
    { value = j.get<float>(); }
)

REGISTER_TYPE_SERIALIZER(double,
    { j = value; },
    { value = j.get<double>(); }
)

REGISTER_TYPE_SERIALIZER(bool,
    { j = value; },
    { value = j.get<bool>(); }
)

REGISTER_TYPE_SERIALIZER(UniformData,
    {
        j = json::object();
        std::visit([&j](auto&& value)
            {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, bool>)
                    j["type"] = "bool";
                else if constexpr (std::is_same_v<T, int>)
                    j["type"] = "int";
                else if constexpr (std::is_same_v<T, float>)
                    j["type"] = "float";
                else if constexpr (std::is_same_v<T, double>)
                    j["type"] = "double";
                else if constexpr (std::is_same_v<T, glm::ivec1>)
                    j["type"] = "ivec1";
                else if constexpr (std::is_same_v<T, glm::vec2>)
                    j["type"] = "vec2";
                else if constexpr (std::is_same_v<T, glm::vec3>)
                    j["type"] = "vec3";
                else if constexpr (std::is_same_v<T, glm::vec4>)
                    j["type"] = "vec4";
                else if constexpr (std::is_same_v<T, glm::mat4>)
                    j["type"] = "mat4";
                else if constexpr (std::is_same_v<T, glm::quat>)
                    j["type"] = "quat";
                else if constexpr (std::is_same_v<T, std::string>)
                    j["type"] = "texture";
                else
                    j["type"] = "unknown";

                std::optional<SerializerInfo> value_serializer = FieldSerializationRegistry::instance().get_serializer(typeid(T));
                if (value_serializer.has_value()) {
                    value_serializer.value().serializable_function(j["value"], &value);
                }
                else {
                    PRINTLN("failed finding a serializer for type");
                    j["value"] = nullptr;
                }
            }, value.value);
        j["location"] = value.location.string;
    },
        {

            /// Actually deserialization becomes extremely tricky... I need to first construct the UniformData, and then fill it in...
            std::string type_str = j["type"].get<std::string>();

            if (type_str == "bool") {
                bool temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(bool));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(bool);
                value.value = temp;
            }
            else if (type_str == "int") {
                int temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(int));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(int);
                value.value = temp;
            }
            else if (type_str == "float") {
                float temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(float));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(float);
                value.value = temp;
            }
            else if (type_str == "double") {
                double temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(double));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(double);
                value.value = temp;
            }
            else if (type_str == "ivec1") {
                glm::ivec1 temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(glm::ivec1));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(glm::ivec1);
                value.value = temp;
            }
            else if (type_str == "vec2") {
                glm::vec2 temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(glm::vec2));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(glm::vec2);
                value.value = temp;
            }
            else if (type_str == "vec3") {
                glm::vec3 temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(glm::vec3));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(glm::vec3);
                value.value = temp;
            }
            else if (type_str == "vec4") {
                glm::vec4 temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(glm::vec4));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(glm::vec4);
                value.value = temp;
            }
            else if (type_str == "mat4") {
                glm::mat4 temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(glm::mat4));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(glm::mat4);
                value.value = temp;
            }
            else if (type_str == "quat") {
                glm::quat temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(glm::quat));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(glm::quat);
                value.value = temp;
            }
            else if (type_str == "texture")
            {
                std::string temp{};
                auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(std::string));
                if (serializer.has_value()) {
                    serializer.value().deserializable_function(j["value"], &temp);
                }
                value.type = typeid(std::string);
                value.value = temp;
            }
            else if (type_str == "unknown") PRINTLN("Found unknown as value");
            value.location = hashed_string_64(j["location"].get<std::string>().c_str());

            PRINTLN("value.value holds type index: {}", value.value.index());
        })

//REGISTER_TYPE_SERIALIZER(UniformData,
//{
//
//    j = json::object();
//    std::string name = value.type.name();
//    PRINTLN("value type {}", name);
//    const void* val = nullptr;
//
//    if (name.find("glm::vec<1,int") != std::string::npos)
//    {
//        j["type"] = "ivec1";
//        val = std::any_cast<glm::ivec1>(&value.value);
//    }
//    else if (name.find("bool") != std::string::npos) {
//        j["type"] = "bool";
//        val = std::any_cast<bool>(&value.value);
//    }
//    else if (name.find("vec<2,float") != std::string::npos) {
//        j["type"] = "vec2";
//        val = std::any_cast<glm::vec2>(&value.value);
//    }
//    else if (name.find("vec<3,float") != std::string::npos) {
//        j["type"] = "vec3";
//        val = std::any_cast<glm::vec3>(&value.value);
//    }
//    else if (name.find("vec<4,float") != std::string::npos) {
//        j["type"] = "vec4";
//        val = std::any_cast<glm::vec4>(&value.value);
//    }
//    else if (name.find("mat<4,4,float") != std::string::npos) {
//        j["type"] = "mat4";
//        val = std::any_cast<glm::mat4>(&value.value);
//    }
//    else if (name.find("float") != std::string::npos || name == "f") {
//        j["type"] = "float";
//        val = std::any_cast<float>(&value.value);
//    }
//    else if (name.find("int") != std::string::npos || name == "i") {
//        j["type"] = "int";
//        val = std::any_cast<int>(&value.value);
//    }
//    else if (name.find("double") != std::string::npos) {
//        j["type"] = "double";
//        val = std::any_cast<double>(&value.value);
//    }
//    else {
//        j["type"] = "unknown";
//        j["raw_name"] = value.type.name();
//    }
//
//    if (val)
//    {
//        std::optional<SerializerInfo> value_serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (value_serializer.has_value())
//        {
//            value_serializer.value().serializable_function(j["value"], val);
//        }
//        else
//        {
//            PRINTLN("failed finding a serializer for UniformData");
//            j["value"] = nullptr;
//        }
//    }
//    else
//    {
//        PRINTLN("No valid any_cast available for writing value of UniformData");
//        j["value"] = nullptr;
//    }
//
//    j["location"] = value.location.string;
//    j["dirty"] = value.dirty;
//    },
//    {
//
//        /// Actually deserialization becomes extremely tricky... I need to first construct the UniformData, and then fill it in...
//            value.value.reset();
//    std::string type_str = j["type"].get<std::string>();
//
//    if (type_str == "ivec1") {
//        value.type = typeid(glm::ivec1);
//        value.value.emplace<glm::ivec1>();
//        void* ptr = std::any_cast<glm::ivec1>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "vec2") {
//        value.type = typeid(glm::vec2);
//        value.value.emplace<glm::vec2>();
//        void* ptr = std::any_cast<glm::vec2>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "vec3") {
//        value.type = typeid(glm::vec3);
//        value.value.emplace<glm::vec3>();
//        void* ptr = std::any_cast<glm::vec3>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "vec4") {
//        value.type = typeid(glm::vec4);
//        value.value.emplace<glm::vec4>();
//        void* ptr = std::any_cast<glm::vec4>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "mat4") {
//        value.type = typeid(glm::mat4);
//        value.value.emplace<glm::mat4>();
//        void* ptr = std::any_cast<glm::mat4>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "float") {
//        value.type = typeid(float);
//        value.value.emplace<float>();
//        void* ptr = std::any_cast<float>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "int") {
//        value.type = typeid(int);
//        value.value.emplace<int>();
//        void* ptr = std::any_cast<int>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "double") {
//        value.type = typeid(double);
//        value.value.emplace<double>();
//        void* ptr = std::any_cast<double>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//    else if (type_str == "bool") {
//        value.type = typeid(bool);
//        value.value.emplace<bool>();
//        void* ptr = std::any_cast<bool>(&value.value);
//        auto serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (serializer.has_value()) {
//            serializer.value().deserializable_function(j["value"], ptr);
//        }
//    }
//        value.location = hashed_string_64(j["location"].get<std::string>().c_str());
//        value.dirty = j["dirty"];
//
//        PRINTLN("value.type: {}", value.type.name());
//        PRINTLN("value.value.type(): {}", value.value.type().name());
//        PRINTLN("Are they equal? {}", value.type == value.value.type());
//    }
//    )

//REGISTER_TYPE_SERIALIZER(UniformDataBase,
//    {
//        j = json::object();
//        //std::optional<SerializerInfo> type_serializer = FieldSerializationRegistry::instance().get_serializer(typeid(std::type_index));
//        //if (type_serializer.has_value())
//        //{
//        //    type_serializer.value().serializable_function(j["type"], &value.type);
//        //}
//        //else
//        //{
//        //    j["type"] = "unknown";
//        //}
//        std::string name = value.type.name();
//        PRINTLN("value type {}", name);
//        if (name.find("int") != std::string::npos || name == "i") {
//            j["type"] = "int";
//        }
//        else if (name.find("double") != std::string::npos) {
//            j["type"] = "double";
//        }
//        else if (name.find("bool") != std::string::npos) {
//            j["type"] = "bool";
//        }
//        else if (name.find("vec<2,float") != std::string::npos) {
//            j["type"] = "vec2";
//        }
//        else if (name.find("vec<3,float") != std::string::npos) {
//            j["type"] = "vec3";
//        }
//        else if (name.find("vec<4,float") != std::string::npos) {
//            j["type"] = "vec4";
//        }
//        else if (name.find("mat<4,float") != std::string::npos) {
//            j["type"] = "mat4";
//        }
//        else if (name.find("float") != std::string::npos || name == "f") {
//            j["type"] = "float";
//        }
//        else {
//            j["type"] = "unknown";
//            j["raw_name"] = value.type.name();
//        }
//        std::optional<SerializerInfo> value_serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (value_serializer.has_value())
//        {
//            PRINTLN("GOT INTO SERIALIZER");
//            value_serializer.value().serializable_function(j["value"], &value.value_ptr);
//        }
//        else
//        {
//            PRINTLN("DID NOT GET INTO SERIALIZER");
//            j["value"] = nullptr;
//        }
//
//        j["name"] = value.name.string;
//        j["dirty"] = value.dirty;
//    },
//    {
//        //std::optional<SerializerInfo> type_serializer = FieldSerializationRegistry::instance().get_serializer(typeid(std::type_index));
//        //if (type_serializer.has_value())
//        //{
//        //    type_serializer.value().deserializable_function(j["type"], &value.type);
//        //}
//        //else
//        //{
//        //    value.type = typeid(NULL);
//        //}
//        if (j["type"].get<std::string>() == "int") {
//            value.type = typeid(int);
//        }
//        else if (j["type"].get<std::string>() == "double") {
//            value.type = typeid(double);
//        }
//        else if (j["type"].get<std::string>() == "bool") {
//            value.type = typeid(bool);
//        }
//        else if (j["type"].get<std::string>() == "vec2") {
//            value.type = typeid(glm::vec2);
//        }
//        else if (j["type"].get<std::string>() == "vec3") {
//            value.type = typeid(glm::vec3);
//        }
//        else if (j["type"].get<std::string>() == "vec4") {
//            value.type = typeid(glm::vec4);
//        }
//        else if (j["type"].get<std::string>() == "mat4") {
//            value.type = typeid(glm::mat4);
//        }
//        else if (j["type"].get<std::string>() == "float") {
//            value.type = typeid(float);
//        }
//        else {
//            value.type = typeid(NULL);  // fallback
//        }
//        std::optional<SerializerInfo> value_serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//        if (value_serializer.has_value())
//        {
//            value_serializer.value().deserializable_function(j["value"], &value.value_ptr);
//        }
//        else
//        {
//            value.value_ptr = nullptr;
//        }
//
//        value.name = hashed_string_64(j["name"].get<std::string>().c_str());
//        value.dirty = j["dirty"];
//    }
//)

//namespace {
//    const bool _serializer_registered_60 = []() 
//        { 
//            FieldSerializationRegistry::instance().register_serializable_field(std::type_index(typeid(UniformCall)), 
//                [](json& j, const void* ptr) 
//                { 
//                    const UniformCall& value = *static_cast<const UniformCall*>(ptr); 
//                    {
//                        j = json::object(); 
//                        std::optional<SerializerInfo> type_serializer = FieldSerializationRegistry::instance().get_serializer(typeid(std::type_index));
//                        if (type_serializer.has_value())
//                        {
//                            type_serializer.value().serializable_function(j["type"], &value.type);
//                        }
//                        else
//                        {
//                            j["type"] = "unknown";
//                        }
//                        std::optional<SerializerInfo> value_serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//                        if (value_serializer.has_value())
//                        {
//                            value_serializer.value().serializable_function(j["value"], &value.value_ptr);
//                        }
//                        else
//                        {
//                            j["value"] = nullptr;
//                        }
//
//                        j["name"] = value.name.string;
//                        j["dirty"] = value.dirty;
//                    }; 
//                },
//                [](const json& j, void* ptr) 
//                { 
//                    UniformCall& value = *static_cast<UniformCall*>(ptr); 
//                    {
//                        std::optional<SerializerInfo> type_serializer = FieldSerializationRegistry::instance().get_serializer(typeid(std::type_index));
//                        if (type_serializer.has_value())
//                        {
//                            type_serializer.value().deserializable_function(j["type"], &value.type);
//                        }
//                        else
//                        {
//                            value.type = typeid(NULL);
//                        }
//                        std::optional<SerializerInfo> value_serializer = FieldSerializationRegistry::instance().get_serializer(value.type);
//                        if (value_serializer.has_value())
//                        {
//                            value_serializer.value().deserializable_function(j["value"], &value.value_ptr);
//                        }
//                        else
//                        {
//                            value.value_ptr = nullptr;
//                        }
//
//                        value.name.string = j["name"];
//                        value.dirty = j["dirty"];
//                    }; 
//                }); return true; 
//        }();
//}
