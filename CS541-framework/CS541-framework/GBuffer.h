#include <glload/gl_3_3.h>
#include <glload/gl_load.hpp>
#include <glm/glm.hpp>
#include <GL/freeglut.h>

enum TEXTURE_TYPE {
	GBUFFER_TEXTURE_TYPE_POSITION,
	GBUFFER_TEXTURE_TYPE_DIFFUSE,
	GBUFFER_TEXTURE_TYPE_NORMAL,
	GBUFFER_TEXTURE_TYPE_TEXCOORD,
	GBUFFER_TEXTURE_TYPE_EYEVEC,
	GBUFFER_TEXTURE_TYPE_LIGHTVEC,
	GBUFFER_TEXTURE_TYPE_SHADOW_CORD,
	GBUFFER_NUM_TEXTURES
};

class GBuffer {
public:



	unsigned int fbo;
	GLuint m_textures[GBUFFER_NUM_TEXTURES];
	GLuint m_depthTexture;
	int width, height;  // Size of the texture.

	void CreateGBuffer(const int w, const int h);
	void Bind();
	void ReadBind();
	void SetReadBuffer(TEXTURE_TYPE);
	void Unbind();
};