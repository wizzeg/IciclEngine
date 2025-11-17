#pragma once

#ifdef __cpp_lib_print
#include <print>
#ifdef _DEBUG
#define PRINTLN(...) std::println(__VA_ARGS__)
#else
#define PRINTLN(...)
#endif
#else
#define PRINTLN(...)
#endif

#define ELEMENTS_IN_ARRAY(a) (sizeof(a)/sizeof(a[0]))