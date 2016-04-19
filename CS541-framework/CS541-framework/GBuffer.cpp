#include "GBuffer.h"


void GBuffer::CreateGBuffer(int height, int width)
{
	// Create the FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

	// Create the gbuffer textures
	glGenTextures(GBUFFER_NUM_TEXTURES, m_textures);
	glGenTextures(1, &m_depthTexture);

	for (unsigned int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0, GL_RGB, GL_FLOAT, NULL);

		//added this when multiple textures were not working
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_textures[i], 0);
	}

	// depth
	glBindTexture(GL_TEXTURE_2D, m_depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
		NULL);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

	GLenum DrawBuffers[] = { 
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4,
		GL_COLOR_ATTACHMENT5,
		GL_COLOR_ATTACHMENT6 };
	glDrawBuffers( 7, DrawBuffers); //4 is size of BrawBuffer[]

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("### GBuffer error, status: 0x%x\n", Status);
	}

	// restore default FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void GBuffer::Bind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
}

void GBuffer::ReadBind()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
}

void GBuffer::SetReadBuffer(TEXTURE_TYPE TextureType)
{
	glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);
}

void GBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}