#pragma once
#include <string>
#include <typeindex>

enum EEditMode
{
	Editable,
	Uneditable,
};

struct FieldInfo /// perhaps make these into imgui drawcalls instead... but then caching becomes problem? unless just include the drawcall as extra?
{
	EEditMode edit_mode;
	std::string name;
	std::type_index type;
	void* value_ptr;
	float imgui_size = 1;
	bool same_line = false;
};
