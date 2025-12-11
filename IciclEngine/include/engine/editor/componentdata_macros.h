#pragma once
#include <engine/editor/component_factory.h>
#include <engine/editor/component_registry.h>
#include <engine/game/components.h>
#include <engine/editor/field_info.h>
#include <string>
#include <vector>

//#define FIELD(component, field) \
//    { EEditMode::Editable, #field, typeid(decltype(component.field)), &component.field, 1.0f, false, false, {} }

#define  FIELD_READONLY_ENUM(label, component, field, ...) \
    { EEditMode::Uneditable, label, typeid(decltype(component.field)), &component.field, 1.0f, false, true, {__VA_ARGS__} }

#define FIELD_ENUM(label, component, field, ...) \
    { EEditMode::Editable, label, typeid(decltype(component.field)), &component.field, 1.0f, false, true, {__VA_ARGS__} }

#define  FIELD_READONLY_DEFAULT(label, component, field) \
    { EEditMode::Uneditable, label, typeid(decltype(component.field)), &component.field, 1.0f, false, false, {} }

#define FIELD_DEFAULT(label, component, field) \
    { EEditMode::Editable, label, typeid(decltype(component.field)), &component.field, 1.0f, false, false, {} }

#define  FIELD_READONLY_CUSTOM(label, component, field, size, sameline) \
    { EEditMode::Uneditable, label, typeid(decltype(component.field)), &component.field, size, sameline, false, {} }

#define FIELD_CUSTOM(label, component, field, size, sameline) \
    { EEditMode::Editable, label, typeid(decltype(component.field)), &component.field, size, sameline, false, {} }

// Helper macro to convert variadic args to vector
#define MAKE_CATEGORY_VECTOR(...) std::vector<std::string>{__VA_ARGS__}

#define REGISTER_COMPONENT(component, categories, ...) \
    namespace { \
        struct component##_Data \
        { \
            component##_Data() \
            { \
                ComponentRegistry::instance().register_component<component>( \
                    #component, \
                    MAKE_CATEGORY_VECTOR categories, \
                    [](void* comp_ptr) -> std::vector<FieldInfo> { \
                        component& comp = *static_cast<component*>(comp_ptr); \
                        return { __VA_ARGS__ }; \
                    } \
                ); \
                ComponentFactory::instance().register_factory<component>(#component); \
            } \
        }; \
        static component##_Data component##_data; \
    }
// the generated function takes in a void ptr that SHOULD be a component pointr of the type of component (TComponent)
// this static casts into comp, which is of component/TComponent. When Registered, comp is used as a variable in the declaration of FIELD_*
// when registering the comopnent. However, when writing the registering macro, comp is not yet defined. comp is defined in the lambda, and
// only later used, it fits into the component slot in the arguments for the FIELD_* macros, and is used as component there, to generate this
// this means, at runtime, everything this function is called, comp is redefined.