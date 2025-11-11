#pragma once
#include <glad/glad.h>
#include <algorithm>
#include <string>

class Texture
{

public:
	Texture(const char* Path);

	Texture(const GLenum WrapX, const GLenum WrapY, const char* Path, GLenum ColorFormat);

	//Texture(const GLenum FilteringMin, const GLenum FilteringMag, const char* Path, GLenum ColorFormat)
	//	: FilteringMin(FilteringMin), FilteringMag(FilteringMag), Path(Path), ColorFormat(ColorFormat) {};

	//Texture(const GLenum WrapX, const GLenum WrapY, const GLenum MipMapFiltering, const char* Path, GLenum ColorFormat)
	//	: WrapX(WrapX), WrapY(WrapY), MipMapFiltering(MipMapFiltering),Path(Path), ColorFormat(ColorFormat) {};

	//Texture(const GLenum WrapX, const GLenum WrapY, const GLenum FilteringMin, const GLenum FilteringMag, const char* Path, GLenum ColorFormat)
	//	: WrapX(WrapX), WrapY(WrapY), FilteringMin(FilteringMin), FilteringMag(FilteringMag), Path(Path), ColorFormat(ColorFormat) {};

	//Texture(const GLenum FilteringMin, const GLenum FilteringMag, const GLenum MipMapFiltering, const char* Path, GLenum ColorFormat)
	//	: FilteringMin(FilteringMin), FilteringMag(FilteringMag), MipMapFiltering(MipMapFiltering), Path(Path), ColorFormat(ColorFormat) {};

	Texture(const GLenum WrapX, const GLenum WrapY, const GLenum FilteringMin, const GLenum FilteringMag, const GLenum MipMapFiltering, const char* Path, GLenum ColorFormat);

	void SetIndex(unsigned int index) { this->index = index; }

	~Texture() {};

	bool virtual Load();
	void virtual Activate();
	
	GLenum WrapX = GL_REPEAT;
	GLenum WrapY = GL_REPEAT;
	GLenum FilteringMin = GL_NEAREST;
	GLenum FilteringMag = GL_LINEAR;
	bool generateMipMap = true;
	GLenum MipMapFiltering = GL_LINEAR_MIPMAP_LINEAR;
	float BorderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f };
	GLenum ColorFormat = GL_RGB;

	bool loaded = false;
	unsigned int TextureID = 0;

	const char* Path;
	unsigned int index = 0;


};

