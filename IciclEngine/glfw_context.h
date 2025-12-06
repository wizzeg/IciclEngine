#pragma once

#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <GLFW/glfw3.h>
#include <engine/utilities/macros.h>
#include <memory>
#include <vector>

#include "frame_buffer.h"
#include <string>

struct GLFWContext
{

	GLFWContext(int a_width, int a_height, const char* a_title, bool a_vsync = false, bool a_depth_test = true) : width(a_width), height(a_height)
	{
		/* Create a windowed mode window and its OpenGL context */
		
		window = glfwCreateWindow(width, height, a_title, NULL, NULL);
		if (!window)
		{
			PRINTLN("NO WINDOW-- - Terminated");
			glfwTerminate();
		}

		/* Make the window's context current */
		glfwMakeContextCurrent(window);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			PRINTLN("Failed to initialize Glad");
		}
		glfwSwapInterval((int)a_vsync);
		if (a_depth_test)
		{
			glEnable(GL_DEPTH_TEST);
		}
	}

	void activate() { glfwMakeContextCurrent(window); }
	void deactivate() { glfwMakeContextCurrent(nullptr); }
	void swap_buffers() { glfwSwapBuffers(window); }
	bool window_should_close() { return glfwWindowShouldClose(window); };
	void clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.3f, 0.2f, 1.0f);
	};
	GLFWwindow* get_window() { return window; }

	bool bind_framebuffer(const std::string a_name)
	{
		for (size_t i = 0; i < frame_buffers.size(); i++)
		{
			if (frame_buffers[i].get_name() == a_name)
			{
				frame_buffers[i].bind();
				return true;
				break;
			}
		}
		return false;
	}
	void unbind_framebuffer()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void resize_framebuffer(const std::string a_name, int a_width, int a_height)
	{
		for (size_t i = 0; i < frame_buffers.size(); i++)
		{
			if (frame_buffers[i].get_name() == a_name)
			{
				frame_buffers[i].resize(a_width, a_height);
				break;
			}
		}
	}

	void create_framebuffer(const std::string name, int a_width, int a_height)
	{
		frame_buffers.emplace_back(name, a_width, a_height);
	}

	unsigned int get_framebuffer_texture(const std::string a_name)
	{
		hashed_string_64 target_hash(a_name.c_str());
		for (size_t i = 0; i < frame_buffers.size(); i++)
		{
			if (frame_buffers[i].get_hashed_name() == target_hash)
			{
				return frame_buffers[i].get_texture();
			}
		}
	}
private:
	std::vector<FrameBuffer> frame_buffers;
	GLFWwindow* window;
	int height;
	int width;
};

