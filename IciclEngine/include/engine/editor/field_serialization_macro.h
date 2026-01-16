#pragma once
#include <engine/editor/field_serialization_registry.h>


//#define REGISTER_TYPE_SERIALIZER(field_type, serializer, deserializer) \
//namespace Serializer_ns_##__LINE__ { \
//    struct Serializer_struct { \
//        Serializer_struct() { \
//            FieldSerializationRegistry::instance().register_serializable_field( \
//                std::type_index(typeid(field_type)), \
//                [](json& j, const void* ptr) { \
//                    const field_type& value = *static_cast<const field_type*>(ptr); \
//                    serializer; \
//                }, \
//                [](json& j, void* ptr) { \
//                    field_type& value = *static_cast<field_type*>(ptr); \
//                    deserializer; \
//                } \
//            ); \
//        } \
//    }; \
//    static Serializer_struct serializer_struct; \
//}

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

// here is big problem!!! need to do typeid -> enum and back (line 34)
#define REGISTER_TYPE_SERIALIZER(field_type, serializer, deserializer) \
namespace { \
    const bool CONCAT(_serializer_registered_, __COUNTER__) = []() { \
        FieldSerializationRegistry::instance().register_serializable_field( \
            std::type_index(typeid(field_type)), \
            [](json& j, const void* ptr) { \
                const field_type& value = *static_cast<const field_type*>(ptr); \
                serializer; \
            }, \
            [](const json& j, void* ptr) { \
                field_type& value = *static_cast<field_type*>(ptr); \
                deserializer; \
            } \
        ); \
        return true; \
    }(); \
}
