#pragma once
#include <engine/core/build_state.h>
#ifdef DRAW_PRINT
#ifdef __cpp_lib_print
#include <print>
#if defined(_DEBUG)
#define PRINTLN(...) std::println(__VA_ARGS__)
#else
#define PRINTLN(...) std::println(__VA_ARGS__)
#endif
#else
#define PRINTLN(...)
#endif
#else
#define PRINTLN(...)
#endif

#define SIZEOF_ELEMENTS_IN_VECTOR(a) (a.size() * sizeof(a[0]))