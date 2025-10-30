#pragma once

#include "RenderPass.h"

class ForwardRenderPass : public RenderPass
{
public:
	//ForwardRenderPass();
	ForwardRenderPass(Shader* aShader);
	~ForwardRenderPass();
	void Render(Scene& scene) override;
	unsigned int RenderFromIndex(Scene& aScene, unsigned int index);
	unsigned int GetShaderProgram() { return shaderProgram; };

protected:
	Shader* shader = nullptr;
	unsigned int shaderProgram = 0;
};

