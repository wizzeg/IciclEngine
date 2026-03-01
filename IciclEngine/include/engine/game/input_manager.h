#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <engine/utilities/macros.h>
#include <unordered_map>
#include <chrono>
#include <atomic>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mutex>
struct EngineContext;
enum class EKey
{
	// Numbers
	Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
	// Letters
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	// Function keys
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	// Special keys
	Space, Enter, Escape, Tab, Backspace, Delete,
	LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt,
	Up, Down, Left, Right, Tilde,
	// Mouse
	LeftMouseButton, RightMouseButton, MiddleMouseButton,
	MAX_VALUE
};

enum class EKeyState
{
	None,
	Pressed,
	Held,
	Released 
};

struct KeyState
{
	EKeyState state = EKeyState::None;
	double press_time = 0.0;
	double release_time = 0.0;
};

struct KeyInput
{
	EKey key;
	KeyState state;
};

struct InputBuffer
{
	InputBuffer()
	{
		for (EKey key = EKey::Zero; key < EKey::MAX_VALUE; key = (EKey)((size_t)key + 1))
			inputs.emplace_back(key);
		mouse_pos = ImGui::GetMousePos();
	}
	std::vector<KeyInput> inputs;
	ImVec2 mouse_pos;
	ImVec2 mouse_delta = ImVec2(0 ,0);
	double time = 0;
};


struct InputManager
{
	friend struct EngineContext;
	static InputManager& instance()
	{
		static InputManager instance;
		return instance;
	}
	void get_mouse_position(float& x, float& y);
	void get_mouse_delta(float& x, float& y);

	bool is_key_pressed(EKey a_key);
	bool is_key_held(EKey a_key);
	bool is_key_released(EKey a_key);
	double key_held_duration(EKey a_key);
	double key_previous_held_duration(EKey a_key);
	// these should be locked behind something
	
	void lock_mouse(float x, float y);
	void unlock_mouse();
	bool is_mouse_locked() const { return mouse_locked; };

private:
	InputManager() = default;
	void update_input();
	void update_key(EKey a_key, ImGuiKey a_imgui_key);
	void update_mouse_button(EKey a_key, ImGuiMouseButton a_mouse_button);
	bool mouse_locked = false;
	int locked_x = 0;
	int locked_y = 0;
	std::mutex mouse_mutex;
	std::unique_ptr<InputBuffer> input_buffer = std::make_unique<InputBuffer>();
};

