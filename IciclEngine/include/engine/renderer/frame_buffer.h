#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstddef>
#include <engine/utilities/macros.h>
#include <string>
#include <engine/utilities/hashed_string_64.h>
#include <vector>

enum EFramebufferType : uint8_t
{
	GBuffer,
	ShadowMap,
	RenderTexture,
	Output
};

struct FrameBuffer
{

	FrameBuffer(const std::string a_name, int a_width, int a_height, EFramebufferType a_type) 
		: width(a_width), height(a_height), hashed_name(a_name.c_str()), type(a_type) // RGB/RGBA/RG/R I think
	{

		framebuffer_object = 0;
		color_buffer = 0;
		position_buffer = 0;
		normal_buffer = 0;
		albedo_specular_buffer = 0;
		depth_rb = 0;
		shadow_depth_buffer = 0;
		orms_buffer = 0;

		glGenFramebuffers(1, &framebuffer_object);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);
		//use this siwtch instead
		switch (a_type)
		{
		case GBuffer:
			create_g_buffer();
			break;
		case ShadowMap:
			create_shadow_buffer();
			break;
		case RenderTexture:
			create_render_texture();
			break;
		case Output:
			create_output_buffer();
			break;
		default:
			break;
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			PRINTLN("Framebuffer {} incomplete! Status: {}", hashed_name.string, glCheckFramebufferStatus(GL_FRAMEBUFFER));
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		PRINTLN("Created framebuffer '{}' with FBO: {}", hashed_name.string, framebuffer_object);
	}

	void bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);
		glViewport(0, 0, width, height);
	}
	void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void clear()
	{
		//glClearColor(0.45f, 0.55f, 0.75f, 0.0f);
		glClearColor(0.45f, 0.55f, 0.75f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
	}

	void resize(int a_width, int a_height)
	{
		if (a_width == width && a_height == height) return;
		width = a_width;
		height = a_height;

		if (position_buffer) { glDeleteTextures(1, &position_buffer); position_buffer = 0; }
		if (normal_buffer) { glDeleteTextures(1, &normal_buffer); normal_buffer = 0; }
		if (albedo_specular_buffer) { glDeleteTextures(1, &albedo_specular_buffer); albedo_specular_buffer = 0; }
		if (color_buffer) { glDeleteTextures(1, &color_buffer); color_buffer = 0; }
		if (shadow_depth_buffer) { glDeleteTextures(1, &shadow_depth_buffer); shadow_depth_buffer = 0; }
		if (orms_buffer) { glDeleteTextures(1, &orms_buffer); orms_buffer = 0; }
		if (depth_rb) { glDeleteRenderbuffers(1, &depth_rb); depth_rb = 0; }
		
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

		switch (type)
		{
		case GBuffer:
			create_g_buffer();
			break;
		case ShadowMap:
			create_shadow_buffer();
			break;
		case RenderTexture:
			create_render_texture();
			break;
		case Output:
			create_output_buffer();
			break;
		default:
			break;
		}
	}

	void create_render_texture()
	{
		if (position_buffer + normal_buffer + albedo_specular_buffer + shadow_depth_buffer > 0) return;
		glGenTextures(1, &color_buffer);
		glBindTexture(GL_TEXTURE_2D, color_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	void create_output_buffer()
	{
		if (position_buffer + normal_buffer + albedo_specular_buffer + shadow_depth_buffer > 0) return;
		glGenTextures(1, &color_buffer);
		glBindTexture(GL_TEXTURE_2D, color_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0);

		glGenRenderbuffers(1, &depth_rb);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	void create_shadow_buffer()
	{
		if (position_buffer + normal_buffer + albedo_specular_buffer + shadow_depth_buffer > 0) return;

		glGenTextures(1, &shadow_depth_buffer);
		glBindBuffer(GL_TEXTURE_2D, shadow_depth_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, width, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth_buffer, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void create_g_buffer()
	{
		if (position_buffer + normal_buffer + albedo_specular_buffer + shadow_depth_buffer > 0) return;
		glGenTextures(1, &position_buffer);
		glBindTexture(GL_TEXTURE_2D, position_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, position_buffer, 0);

		glGenTextures(1, &normal_buffer);
		glBindTexture(GL_TEXTURE_2D, normal_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_buffer, 0);

		glGenTextures(1, &albedo_specular_buffer);
		glBindTexture(GL_TEXTURE_2D, albedo_specular_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedo_specular_buffer, 0);

		glGenTextures(1, &orms_buffer);
		glBindTexture(GL_TEXTURE_2D, orms_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, orms_buffer, 0);

		glGenRenderbuffers(1, &depth_rb);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

		GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, attachments);
	}

	void delete_buffers()
	{
		if (position_buffer) { glDeleteTextures(1, &position_buffer); position_buffer = 0; }
		if (normal_buffer) { glDeleteTextures(1, &normal_buffer); normal_buffer = 0; }
		if (albedo_specular_buffer) { glDeleteTextures(1, &albedo_specular_buffer); albedo_specular_buffer = 0; }
		if (color_buffer) { glDeleteTextures(1, &color_buffer); color_buffer = 0; }
		if (shadow_depth_buffer) { glDeleteTextures(1, &shadow_depth_buffer); shadow_depth_buffer = 0; }
		if (orms_buffer) { glDeleteTextures(1, &orms_buffer); orms_buffer = 0; }
		if (depth_rb) { glDeleteRenderbuffers(1, &depth_rb); depth_rb = 0; }
	}


	const std::string get_name() const { return hashed_name.string; }
	const hashed_string_64 get_hashed_name() const { return hashed_name; }
	const uint64_t get_hash() const { return hashed_name.hash; }
	int get_width() const { return width; }
	int get_height() const { return height; }

	GLuint get_position_texture() const { return position_buffer; }
	GLuint get_normal_texture() const { return normal_buffer; }
	GLuint get_albedo_spec_texture() const { return albedo_specular_buffer; }
	GLuint get_orms_texture() const { return orms_buffer; }
	GLuint get_shadow_depth_texture() const { return shadow_depth_buffer; }
	GLuint get_color_texture() const { return color_buffer; }
	std::vector<GLuint> get_textures() { return { position_buffer, normal_buffer, albedo_specular_buffer, shadow_depth_buffer, color_buffer }; }
private:
	hashed_string_64 hashed_name;

	// keep for all
	EFramebufferType type;
	GLuint framebuffer_object = 0;

	// for output and rendertexture
	

	// for the deffered renderer
	GLuint color_buffer = 0;
	GLuint position_buffer = 0;
	GLuint normal_buffer = 0;
	GLuint albedo_specular_buffer = 0;
	GLuint orms_buffer = 0;

	// for deffered renderer, renderbuffer, just works for the depth test
	GLuint depth_rb = 0;
	

	// shadow depth buffer, for the shadow mapping
	GLuint shadow_depth_buffer = 0;

	int width = 1;
	int height = 1;
};

