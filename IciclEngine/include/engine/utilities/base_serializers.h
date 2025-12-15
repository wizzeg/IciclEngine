//#pragma once
//
//#include <engine/editor/field_serialization_macro.h>
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>
//#include <glm/gtc/quaternion.hpp>
//
//REGISTER_TYPE_SERIALIZER(glm::vec3,
//    { j = json::array({value.x, value.y, value.z}); },
//    {
//        if (j.is_array() && j.size() == 3) {
//            value.x = j[0].get<float>();
//            value.y = j[1].get<float>();
//            value.z = j[2].get<float>();
//        }
//    }
//)
//
//
//
//REGISTER_TYPE_SERIALIZER(glm::vec4,
//    { j = json::array({value.x, value.y, value.z, value.w}); },
//    {
//        if (j.is_array() && j.size() == 4) {
//            value.x = j[0].get<float>();
//            value.y = j[1].get<float>();
//            value.z = j[2].get<float>();
//            value.w = j[3].get<float>();
//        }
//    }
//)
//
//REGISTER_TYPE_SERIALIZER(glm::vec2,
//    { j = json::array({value.x, value.y}); },
//        {
//            if (j.is_array() && j.size() == 2) {
//                value.x = j[0].get<float>();
//                value.y = j[1].get<float>();
//            }
//        }
//)
//
//REGISTER_TYPE_SERIALIZER(glm::quat,
//    { j = json::array({value.x, value.y, value.z, value.w}); },
//        {
//            if (j.is_array() && j.size() == 4) {
//                value.x = j[0].get<float>();
//                value.y = j[1].get<float>();
//                value.z = j[2].get<float>();
//                value.w = j[3].get<float>();
//            }
//        }
//)
//
//REGISTER_TYPE_SERIALIZER(glm::mat4,
//    {
//        j = json::array();
//        for (int i = 0; i < 4; i++) {
//            for (int k = 0; k < 4; k++) {
//                j.push_back(value[i][k]);
//            }
//        }
//    },
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
//)
//
//REGISTER_TYPE_SERIALIZER(std::string,
//    { j = value; },
//    { value = j.get<std::string>(); }
//)
//
//REGISTER_TYPE_SERIALIZER(entt::entity,
//    { j = static_cast<uint32_t>(value); },
//    { value = static_cast<entt::entity>(j.get<uint32_t>()); }
//)
//
//REGISTER_TYPE_SERIALIZER(hashed_string_64,
//    { j = value.string; },
//    { value = hashed_string_64(j.get<std::string>().c_str()); }
//)
//
//REGISTER_TYPE_SERIALIZER(int,
//    { j = value; },
//    { value = j.get<int>(); }
//)
//
//REGISTER_TYPE_SERIALIZER(uint32_t,
//    { j = value; },
//    { value = j.get<uint32_t>(); }
//)
//
//REGISTER_TYPE_SERIALIZER(float,
//    { j = value; },
//    { value = j.get<float>(); }
//)
//
//REGISTER_TYPE_SERIALIZER(double,
//    { j = value; },
//    { value = j.get<double>(); }
//)
//
//REGISTER_TYPE_SERIALIZER(bool,
//    { j = value; },
//    { value = j.get<bool>(); }
//)
//
//REGISTER_TYPE_SERIALIZER(int64_t,
//    { j = value; },
//    { value = j.get<int64_t>(); }
//)
//
//REGISTER_TYPE_SERIALIZER(uint64_t,
//    { j = value; },
//    { value = j.get<uint64_t>(); }
//)