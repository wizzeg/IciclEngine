#pragma once

#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <GLFW/glfw3.h>
#include <engine/utilities/macros.h>
#include <memory>

struct GLContext
{

	GLContext(int a_width, int a_height, const char* a_title, bool a_vsync = 0)
	{
		/* Create a windowed mode window and its OpenGL context */
		
		window = glfwCreateWindow(a_width, a_height, a_title, NULL, NULL);
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

private:
	GLFWwindow* window;
};

