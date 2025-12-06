#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstddef>
#include <engine/utilities/macros.h>
#include <string>
#include <engine/utilities/hashed_string_64.h>

struct FrameBuffer
{
	FrameBuffer(const std::string a_name, int a_width, int a_height) 
		: width(a_width), height(a_height), name(a_name), hashed_name(a_name.c_str()) // RGB/RGBA/RG/R I think
	{
		glGenFramebuffers(1, &frame_buffer);
		PRINTLN("generated frame buffer {}", frame_buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
		// Create a texture for color attachment
		glGenTextures(1, &texture_color_buffer);
		glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_color_buffer, 0);

		// Create a renderbuffer object for depth and stencil attachment
		glGenRenderbuffers(1, &render_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_buffer);

		// Check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			PRINTLN("Something went wrong when making new framebuffer");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
		glViewport(0, 0, width, height);
	}
	void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void resize(int a_width, int a_height)
	{
		if (a_width == width && a_height == height) return;
		width = a_width;
		height = a_height;

		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

		// Resize color texture (pass NULL data to reallocate empty storage)
		glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Resize depth/stencil renderbuffer
		glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// Re-check completeness
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			// Handle error
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	const std::string get_name() const { return name; }
	const hashed_string_64 get_hashed_name() const { return hashed_name; }
	int get_width() const { return width; }
	int get_height() const { return height; }
	GLuint get_texture() { return texture_color_buffer; }
private:
	std::string name;
	hashed_string_64 hashed_name;

	GLuint texture_color_buffer = 0;
	GLuint render_buffer = 0;
	GLuint frame_buffer = 0;

	int width = 1;
	int height = 1;
};

