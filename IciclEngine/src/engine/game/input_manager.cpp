#include <engine/game/input_manager.h>

void InputManager::update_input()
{

    input_buffer->time = ImGui::GetTime();
    if (mouse_locked)
    {
        input_buffer->mouse_delta = ImGui::GetMousePos() - ImVec2(locked_x, locked_y);
        SetCursorPos((int)locked_x, (int)locked_y);
    }
    else
    {
        input_buffer->mouse_delta = ImGui::GetMousePos() - input_buffer->mouse_pos;
    }
    input_buffer->mouse_pos = ImGui::GetMousePos();

    // Process keyboard input
    update_key(EKey::Zero, ImGuiKey_0);
    update_key(EKey::One, ImGuiKey_1);
    update_key(EKey::Two, ImGuiKey_2);
    update_key(EKey::Three, ImGuiKey_3);
    update_key(EKey::Four, ImGuiKey_4);
    update_key(EKey::Five, ImGuiKey_5);
    update_key(EKey::Six, ImGuiKey_6);
    update_key(EKey::Seven, ImGuiKey_7);
    update_key(EKey::Eight, ImGuiKey_8);
    update_key(EKey::Nine, ImGuiKey_9);

    update_key(EKey::A, ImGuiKey_A);
    update_key(EKey::B, ImGuiKey_B);
    update_key(EKey::C, ImGuiKey_C);
    update_key(EKey::D, ImGuiKey_D);
    update_key(EKey::E, ImGuiKey_E);
    update_key(EKey::F, ImGuiKey_F);
    update_key(EKey::G, ImGuiKey_G);
    update_key(EKey::H, ImGuiKey_H);
    update_key(EKey::I, ImGuiKey_I);
    update_key(EKey::J, ImGuiKey_J);
    update_key(EKey::K, ImGuiKey_K);
    update_key(EKey::L, ImGuiKey_L);
    update_key(EKey::M, ImGuiKey_M);
    update_key(EKey::N, ImGuiKey_N);
    update_key(EKey::O, ImGuiKey_O);
    update_key(EKey::P, ImGuiKey_P);
    update_key(EKey::Q, ImGuiKey_Q);
    update_key(EKey::R, ImGuiKey_R);
    update_key(EKey::S, ImGuiKey_S);
    update_key(EKey::T, ImGuiKey_T);
    update_key(EKey::U, ImGuiKey_U);
    update_key(EKey::V, ImGuiKey_V);
    update_key(EKey::W, ImGuiKey_W);
    update_key(EKey::X, ImGuiKey_X);
    update_key(EKey::Y, ImGuiKey_Y);
    update_key(EKey::Z, ImGuiKey_Z);

    update_key(EKey::F1, ImGuiKey_F1);
    update_key(EKey::F2, ImGuiKey_F2);
    update_key(EKey::F3, ImGuiKey_F3);
    update_key(EKey::F4, ImGuiKey_F4);
    update_key(EKey::F5, ImGuiKey_F5);
    update_key(EKey::F6, ImGuiKey_F6);
    update_key(EKey::F7, ImGuiKey_F7);
    update_key(EKey::F8, ImGuiKey_F8);
    update_key(EKey::F9, ImGuiKey_F9);
    update_key(EKey::F10, ImGuiKey_F10);
    update_key(EKey::F11, ImGuiKey_F11);
    update_key(EKey::F12, ImGuiKey_F12);

    update_key(EKey::Space, ImGuiKey_Space);
    update_key(EKey::Enter, ImGuiKey_Enter);
    update_key(EKey::Escape, ImGuiKey_Escape);
    update_key(EKey::Tab, ImGuiKey_Tab);
    update_key(EKey::Backspace, ImGuiKey_Backspace);
    update_key(EKey::Delete, ImGuiKey_Delete);

    update_key(EKey::LeftShift, ImGuiKey_LeftShift);
    update_key(EKey::RightShift, ImGuiKey_RightShift);
    update_key(EKey::LeftControl, ImGuiKey_LeftCtrl);
    update_key(EKey::RightControl, ImGuiKey_RightCtrl);
    update_key(EKey::LeftAlt, ImGuiKey_LeftAlt);
    update_key(EKey::RightAlt, ImGuiKey_RightAlt);

    update_key(EKey::Up, ImGuiKey_UpArrow);
    update_key(EKey::Down, ImGuiKey_DownArrow);
    update_key(EKey::Left, ImGuiKey_LeftArrow);
    update_key(EKey::Right, ImGuiKey_RightArrow);
    update_key(EKey::Tilde, ImGuiKey_GraveAccent);

    // Process mouse buttons
    update_mouse_button(EKey::LeftMouseButton, ImGuiMouseButton_Left);
    update_mouse_button(EKey::RightMouseButton, ImGuiMouseButton_Right);
    update_mouse_button(EKey::MiddleMouseButton, ImGuiMouseButton_Middle);
}

void InputManager::update_key(EKey a_key, ImGuiKey a_imgui_key)
{
	if (a_key > EKey::Tilde) return;
	bool key_is_down = ImGui::IsKeyDown(a_imgui_key);
	KeyState& key_state = input_buffer->inputs[(size_t)a_key].state;


	if (key_is_down && (key_state.state == EKeyState::Released))
	{
		key_state.state = EKeyState::Pressed;
		key_state.press_time = input_buffer->time;
	}
    else if (key_is_down && (key_state.state == EKeyState::Pressed))
    {
        key_state.state = EKeyState::Held;
    }
	else if (!key_is_down && (key_state.state == EKeyState::Pressed || key_state.state == EKeyState::Held))
	{

		key_state.state = EKeyState::Released;
		key_state.release_time = input_buffer->time;
	}
    else if (key_state.state == EKeyState::None)
    {
        key_state.state = EKeyState::Released;
        key_state.press_time = input_buffer->time;
        key_state.release_time = input_buffer->time;
    }
    
}

void InputManager::update_mouse_button(EKey a_key, ImGuiMouseButton a_mouse_button)
{
	if (a_key <= EKey::RightAlt) return;
	bool button_is_down = ImGui::IsMouseDown(a_mouse_button);
	KeyState& key_state = input_buffer->inputs[(size_t)a_key].state;

	if (button_is_down && (key_state.state == EKeyState::None || key_state.state == EKeyState::Released))
	{
		key_state.state = EKeyState::Pressed;
		key_state.press_time = input_buffer->time;
	}
    else if (button_is_down && (key_state.state == EKeyState::Pressed))
    {
        key_state.state = EKeyState::Held;
    }
	else if (!button_is_down && (key_state.state == EKeyState::Pressed || key_state.state == EKeyState::Held || key_state.state == EKeyState::None))
	{
		key_state.state = EKeyState::Released;
		key_state.release_time = input_buffer->time;
	}
}

void InputManager::lock_mouse(float x, float y)
{
    std::lock_guard mouse_lock(mouse_mutex);
    mouse_locked = true;
    locked_x = x;
    locked_y = y;
}

void InputManager::unlock_mouse()
{
    std::lock_guard mouse_lock(mouse_mutex);
    mouse_locked = false;
}

void InputManager::get_mouse_position(float& x, float& y)
{
    x = input_buffer->mouse_pos.x;
    y = input_buffer->mouse_pos.y;
}

void InputManager::get_mouse_delta(float& x, float& y)
{
    x = input_buffer->mouse_delta.x;
    y = input_buffer->mouse_delta.y;
}

bool InputManager::is_key_pressed(EKey a_key)
{
    return input_buffer->inputs[(size_t)a_key].state.state == EKeyState::Pressed;
}

bool InputManager::is_key_held(EKey a_key)
{
    return input_buffer->inputs[(size_t)a_key].state.state == EKeyState::Held;
}

bool InputManager::is_key_released(EKey a_key)
{
    return input_buffer->inputs[(size_t)a_key].state.state == EKeyState::Released;
}

double InputManager::key_held_duration(EKey a_key)
{
    if (input_buffer->inputs[(size_t)a_key].state.state == EKeyState::Held || input_buffer->inputs[(size_t)a_key].state.state == EKeyState::Pressed)
        return input_buffer->time - input_buffer->inputs[(size_t)a_key].state.press_time;
    return 0;
}

double InputManager::key_previous_held_duration(EKey a_key)
{
    if (input_buffer->inputs[(size_t)a_key].state.state == EKeyState::Released || input_buffer->inputs[(size_t)a_key].state.state == EKeyState::None)
    {
        return input_buffer->inputs[(size_t)a_key].state.release_time - input_buffer->inputs[(size_t)a_key].state.press_time;
    }
    return 0;
}

