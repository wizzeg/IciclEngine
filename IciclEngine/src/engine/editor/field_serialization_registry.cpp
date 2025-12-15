//#include <engine/editor/field_serialization_registry.h>
//#include <engine/editor/field_serialization_macro.h>
//#include <glm/glm.hpp>
//#include <engine/utilities/entt_modified.h>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/quaternion.hpp>
//#include <nlohmann/json.hpp>
//using json = nlohmann::json;
//
//    REGISTER_TYPE_SERIALIZER(glm::vec3,
//        { j = json::array({value.x, value.y, value.z}); },
//        {
//            if (j.is_array() && j.size() == 3) {
//                value.x = j[0].get<float>();
//                value.y = j[1].get<float>();
//                value.z = j[2].get<float>();
//            }
//        }
//    )
//
//
//    REGISTER_TYPE_SERIALIZER(glm::vec4,
//        { j = json::array({value.x, value.y, value.z, value.w}); },
//        {
//            if (j.is_array() && j.size() == 4) {
//                value.x = j[0].get<float>();
//                value.y = j[1].get<float>();
//                value.z = j[2].get<float>();
//                value.w = j[3].get<float>();
//            }
//        }
//    )
//
//    REGISTER_TYPE_SERIALIZER(glm::vec2,
//        { j = json::array({value.x, value.y}); },
//        {
//            if (j.is_array() && j.size() == 2) {
//                value.x = j[0].get<float>();
//                value.y = j[1].get<float>();
//            }
//        }
//    )
//
//    REGISTER_TYPE_SERIALIZER(glm::quat,
//        { j = json::array({value.x, value.y, value.z, value.w}); },
//        {
//            if (j.is_array() && j.size() == 4) {
//                value.x = j[0].get<float>();
//                value.y = j[1].get<float>();
//                value.z = j[2].get<float>();
//                value.w = j[3].get<float>();
//            }
//        }
//    )
//
//    REGISTER_TYPE_SERIALIZER(glm::mat4,
//        {
//            j = json::array();
//            for (int i = 0; i < 4; i++) {
//                for (int k = 0; k < 4; k++) {
//                    j.push_back(value[i][k]);
//                }
//            }
//        },
//        {
//            if (j.is_array() && j.size() == 16) {
//                int idx = 0;
//                for (int i = 0; i < 4; i++) {
//                    for (int k = 0; k < 4; k++) {
//                        value[i][k] = j[idx++].get<float>();
//                    }
//                }
//            }
//        }
//    )
//
//    REGISTER_TYPE_SERIALIZER(std::string,
//        { j = value; },
//        { value = j.get<std::string>(); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(entt::entity,
//        { j = static_cast<uint32_t>(value); },
//        { value = static_cast<entt::entity>(j.get<uint32_t>()); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(hashed_string_64,
//        { j = value.string; },
//        { value = hashed_string_64(j.get<std::string>().c_str()); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(int,
//        { j = value; },
//        { value = j.get<int>(); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(uint32_t,
//        { j = value; },
//        { value = j.get<uint32_t>(); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(float,
//        { j = value; },
//        { value = j.get<float>(); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(double,
//        { j = value; },
//        { value = j.get<double>(); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(bool,
//        { j = value; },
//        { value = j.get<bool>(); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(int64_t,
//        { j = value; },
//        { value = j.get<int64_t>(); }
//    )
//
//    REGISTER_TYPE_SERIALIZER(uint64_t,
//        { j = value; },
//        { value = j.get<uint64_t>(); }
//    )
    //auto& registry = FieldSerializationRegistry::instance();

    //// glm::vec3
    //registry.register_serializable_field(
    //    std::type_index(typeid(glm::vec3)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const glm::vec3*>(ptr);
    //        j = json::array({ value.x, value.y, value.z });
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<glm::vec3*>(ptr);
    //        if (j.is_array() && j.size() == 3) {
    //            value.x = j[0].get<float>();
    //            value.y = j[1].get<float>();
    //            value.z = j[2].get<float>();
    //        }
    //    },
    //    sizeof(glm::vec3)
    //);

    //// glm::vec4
    //registry.register_serializable_field(
    //    std::type_index(typeid(glm::vec4)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const glm::vec4*>(ptr);
    //        j = json::array({ value.x, value.y, value.z, value.w });
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<glm::vec4*>(ptr);
    //        if (j.is_array() && j.size() == 4) {
    //            value.x = j[0].get<float>();
    //            value.y = j[1].get<float>();
    //            value.z = j[2].get<float>();
    //            value.w = j[3].get<float>();
    //        }
    //    },
    //    sizeof(glm::vec4)
    //);

    //// glm::vec2
    //registry.register_serializable_field(
    //    std::type_index(typeid(glm::vec2)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const glm::vec2*>(ptr);
    //        j = json::array({ value.x, value.y });
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<glm::vec2*>(ptr);
    //        if (j.is_array() && j.size() == 2) {
    //            value.x = j[0].get<float>();
    //            value.y = j[1].get<float>();
    //        }
    //    },
    //    sizeof(glm::vec2)
    //);

    //// glm::quat
    //registry.register_serializable_field(
    //    std::type_index(typeid(glm::quat)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const glm::quat*>(ptr);
    //        j = json::array({ value.x, value.y, value.z, value.w });
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<glm::quat*>(ptr);
    //        if (j.is_array() && j.size() == 4) {
    //            value.x = j[0].get<float>();
    //            value.y = j[1].get<float>();
    //            value.z = j[2].get<float>();
    //            value.w = j[3].get<float>();
    //        }
    //    },
    //    sizeof(glm::quat)
    //);

    //// glm::mat4
    //registry.register_serializable_field(
    //    std::type_index(typeid(glm::mat4)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const glm::mat4*>(ptr);
    //        j = json::array();
    //        for (int i = 0; i < 4; i++) {
    //            for (int k = 0; k < 4; k++) {
    //                j.push_back(value[i][k]);
    //            }
    //        }
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<glm::mat4*>(ptr);
    //        if (j.is_array() && j.size() == 16) {
    //            int idx = 0;
    //            for (int i = 0; i < 4; i++) {
    //                for (int k = 0; k < 4; k++) {
    //                    value[i][k] = j[idx++].get<float>();
    //                }
    //            }
    //        }
    //    },
    //    sizeof(glm::mat4)
    //);

    //// std::string
    //registry.register_serializable_field(
    //    std::type_index(typeid(std::string)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const std::string*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<std::string*>(ptr);
    //        value = j.get<std::string>();
    //    },
    //    sizeof(std::string)
    //);

    //// entt::entity
    //registry.register_serializable_field(
    //    std::type_index(typeid(entt::entity)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const entt::entity*>(ptr);
    //        j = static_cast<uint32_t>(value);
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<entt::entity*>(ptr);
    //        value = static_cast<entt::entity>(j.get<uint32_t>());
    //    },
    //    sizeof(entt::entity)
    //);

    //// hashed_string_64
    //registry.register_serializable_field(
    //    std::type_index(typeid(hashed_string_64)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const hashed_string_64*>(ptr);
    //        j = value.string;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<hashed_string_64*>(ptr);
    //        value = hashed_string_64(j.get<std::string>().c_str());
    //    },
    //    sizeof(hashed_string_64)
    //);

    //// int
    //registry.register_serializable_field(
    //    std::type_index(typeid(int)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const int*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<int*>(ptr);
    //        value = j.get<int>();
    //    },
    //    sizeof(int)
    //);

    //// uint32_t
    //registry.register_serializable_field(
    //    std::type_index(typeid(uint32_t)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const uint32_t*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<uint32_t*>(ptr);
    //        value = j.get<uint32_t>();
    //    },
    //    sizeof(uint32_t)
    //);

    //// float
    //registry.register_serializable_field(
    //    std::type_index(typeid(float)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const float*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<float*>(ptr);
    //        value = j.get<float>();
    //    },
    //    sizeof(float)
    //);

    //// double
    //registry.register_serializable_field(
    //    std::type_index(typeid(double)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const double*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<double*>(ptr);
    //        value = j.get<double>();
    //    },
    //    sizeof(double)
    //);

    //// bool
    //registry.register_serializable_field(
    //    std::type_index(typeid(bool)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const bool*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<bool*>(ptr);
    //        value = j.get<bool>();
    //    },
    //    sizeof(bool)
    //);

    //// int64_t
    //registry.register_serializable_field(
    //    std::type_index(typeid(int64_t)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const int64_t*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<int64_t*>(ptr);
    //        value = j.get<int64_t>();
    //    },
    //    sizeof(int64_t)
    //);

    //// uint64_t
    //registry.register_serializable_field(
    //    std::type_index(typeid(uint64_t)),
    //    [](json& j, const void* ptr) {
    //        const auto& value = *static_cast<const uint64_t*>(ptr);
    //        j = value;
    //    },
    //    [](const json& j, void* ptr) {
    //        auto& value = *static_cast<uint64_t*>(ptr);
    //        value = j.get<uint64_t>();
    //    },
    //    sizeof(uint64_t)
    //);
//};
