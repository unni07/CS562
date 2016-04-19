//////////////////////////////////////////////////////////////////////
// Defines and draws a scene.  There are two main procedures here:
//
// (1) void InitializeScene(Scene &scene): is called before the main
// loop is entered, and is expected to setup all OpenGL objects needed
// for the rendering loop.
//
// (2) void DrawScene(Scene &scene): Is called each time the screen
// needs to be refreshed. (Which will be whenever the screen is
// expose/resized, anytime the preiodic animation clock ticks, or
// anytime there is user mouse/keyboard interaction.
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////

#include "math.h"
#include <fstream>
#include <stdlib.h>

#include <glload/gl_3_3.h>
#include <glload/gl_load.hpp>
#include <GL/freeglut.h>
#include <glimg/glimg.h>
#include <glm/gtc/matrix_inverse.hpp>

#include "shader.h"
#include "fbo.h"
#include "GBuffer.h"
#include "models.h"
#include "scene.h"

using namespace glm;

static mat4 Identity(1.0f);

////////////////////////////////////////////////////////////////////////
// This macro makes it easy to sprinkle checks for OpenGL errors
// throught your code.  Most OpenGL calls can record errors, and a
// careful programmer will check the error status *often*, perhaps as
// often as after each OpenGL call.  At the very least, once per
// refresh will tell you if something is going wrong.
#define CHECKERROR {int err = glGetError(); if (err) { fprintf(stderr, "OpenGL error (at line %d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }

float ambientColor[3] = {0.3f, 0.3f, 0.2f};
float lightColor[3] = {0.8f, 0.8f, 0.8f};

const float PI = 3.14159f;
const float rad = PI/180.0f;

FBO *fbo_shadow = new FBO();
FBO *fbo_reflection_top = new FBO();
FBO *fbo_reflection_bottom = new FBO();
GBuffer *gbuff = new GBuffer();

bool lightPass = false;
mat4x4 ShadowWorldView;
mat4x4 ShadowMatrix;
float skyDomeSize = 300;

mat4x4 groundModelTr;
////////////////////////////////////////////////////////////////////////
// A small function to provide a more friendly method of defining
// colors.  The parameters are hue (0..1: fraction of distance around
// the color wheel; red at 0, green at 1/3, blue at 2/3), saturation
// (0..1: achromatic to saturated), and value (0..1: brightness).
#define ret(r,g,b) {c[0]=r; c[1]=g; c[2]=b; }
void HSV2RGB(const float h, const float s, const float v, float *c)
{
    if (s == 0.0)
        ret(v,v,v)
    else {
        int i = (int)(h*6.0);
        float f = (h*6.0f) - i;
        float p = v*(1.0f - s);
        float q = v*(1.0f - s*f);
        float t = v*(1.0f - s*(1.0f-f));
        if (i%6 == 0)
            ret(v,t,p)
        else if (i == 1)
            ret(q,v,p)
        else if (i == 2)
            ret(p,v,t)
        else if (i == 3)
            ret(p,q,v)
        else if (i == 4)
            ret(t,p,v)
        else if (i == 5)
            ret(v,p,q) }
}

////////////////////////////////////////////////////////////////////////
// InitializeScene is called once during setup to create all the
// textures, model VAOs, render target FBOs, and shader programs as
// well as a number of other parameters.
void InitializeScene(Scene &scene)
{
    CHECKERROR;
	scene.RenderMode = 1;
    scene.mode = 0;
    scene.nSpheres = 16;
    scene.drawSpheres = true;
	scene.drawTeapot = true;
    scene.drawGround = true;
	scene.drawDragon = true;
	scene.drawHappy = true;
	scene.drawBunny = true;

    // Set the initial viewing transformation parameters
    scene.front = 0.10f;
    scene.eyeSpin = -150.0f;
    scene.eyeTilt = -70.0f;
    scene.translatex = 0.0f;
    scene.translatey = 0.0f;
    scene.zoom = 90.0f;

    // Set the initial light position parammeters
    scene.lightSpin = -50;
    scene.lightTilt = -50.0f;
    scene.lightDist = 90.0f;
	
	/*
	scene.lightSpin = -150.0f;
	scene.lightTilt = -70.0f;
	scene.lightDist = 90.0f;
	*/
	//used for shadow map and reflection
	fbo_shadow->CreateFBO(1024, 1024);
	fbo_reflection_top->CreateFBO(512, 512);
	fbo_reflection_bottom->CreateFBO(512, 512);
	gbuff->CreateGBuffer(1024, 1024);

    // Enable OpenGL depth-testing
    glEnable(GL_DEPTH_TEST);

    // Create the scene models
    scene.centralPolygons =  new Teapot(62);

	/*scene.dragonPolygons = new Ply("dragon.ply");

	if (scene.drawHappy){
		scene.happyPolygons = new Ply("happy.ply");
	}
	if (scene.drawBunny){
		scene.bunnyPolygons = new Ply("bunny.ply");
	}*/
    scene.spherePolygons = new Sphere(32);
    scene.groundPolygons= new Ground(50.0, 100.0);
	scene.skyDome = new Sphere(32);
	scene.screenQuad = new ScreenQuad();

	groundModelTr = translate(0.0f, 0.0f, 0.0f);

    float s = 20.0/scene.centralPolygons->size;
	s = 2.0f;
	float yPos = -4.0;
    scene.centralTr =
		scale(Identity, s,s,s)
        *translate(-scene.centralPolygons->center);


		

    // Create the lighting shader program from source code files.
    scene.lightingShader.CreateProgram();
    scene.lightingShader.CreateShader("lighting.vert", GL_VERTEX_SHADER);
    scene.lightingShader.CreateShader("lighting.frag", GL_FRAGMENT_SHADER);
    glBindAttribLocation(scene.lightingShader.program, 0, "vertex");
    glBindAttribLocation(scene.lightingShader.program, 1, "vertexNormal");
    glBindAttribLocation(scene.lightingShader.program, 2, "vertexTexture");
    glBindAttribLocation(scene.lightingShader.program, 3, "vertexTangent");
    scene.lightingShader.LinkProgram();

	// Create the shadow shader program from source code files.
	scene.shadowShader.CreateProgram();
	scene.shadowShader.CreateShader("shadow.vert", GL_VERTEX_SHADER);
	scene.shadowShader.CreateShader("shadow.frag", GL_FRAGMENT_SHADER);
	glBindAttribLocation(scene.shadowShader.program, 0, "vertex");
	glBindAttribLocation(scene.shadowShader.program, 1, "vertexNormal");
	glBindAttribLocation(scene.shadowShader.program, 2, "vertexTexture");
	glBindAttribLocation(scene.shadowShader.program, 3, "vertexTangent");
	scene.shadowShader.LinkProgram();

	// Create the reflection shader program from source code files.
	scene.reflectionShader.CreateProgram();
	scene.reflectionShader.CreateShader("reflection.vert", GL_VERTEX_SHADER);
	scene.reflectionShader.CreateShader("reflection.frag", GL_FRAGMENT_SHADER);
	glBindAttribLocation(scene.reflectionShader.program, 0, "vertex");
	glBindAttribLocation(scene.reflectionShader.program, 1, "vertexNormal");
	glBindAttribLocation(scene.reflectionShader.program, 2, "vertexTexture");
	glBindAttribLocation(scene.reflectionShader.program, 3, "vertexTangent");
	scene.reflectionShader.LinkProgram();

	// Create the Gbuffer shader program from source code files.
	scene.GBufferShader.CreateProgram();
	scene.GBufferShader.CreateShader("GBuffer.vert", GL_VERTEX_SHADER);
	scene.GBufferShader.CreateShader("GBuffer.frag", GL_FRAGMENT_SHADER);
	glBindAttribLocation(scene.GBufferShader.program, 0, "vertex");
	glBindAttribLocation(scene.GBufferShader.program, 1, "vertexNormal");
	glBindAttribLocation(scene.GBufferShader.program, 2, "vertexTexture");
	glBindAttribLocation(scene.GBufferShader.program, 3, "vertexTangent");
	scene.GBufferShader.LinkProgram();

	// Create the Gbuffer shader program from source code files.
	scene.AmbiantShader.CreateProgram();
	scene.AmbiantShader.CreateShader("AmbiantPass.vert", GL_VERTEX_SHADER);
	scene.AmbiantShader.CreateShader("AmbiantPass.frag", GL_FRAGMENT_SHADER);
	glBindAttribLocation(scene.AmbiantShader.program, 0, "vertex");
	glBindAttribLocation(scene.AmbiantShader.program, 1, "vertexNormal");
	glBindAttribLocation(scene.AmbiantShader.program, 2, "vertexTexture");
	glBindAttribLocation(scene.AmbiantShader.program, 3, "vertexTangent");
	scene.AmbiantShader.LinkProgram();

	
	scene.GlobalLightingShader.CreateProgram();
	scene.GlobalLightingShader.CreateShader("GlobalLighting.vert", GL_VERTEX_SHADER);
	scene.GlobalLightingShader.CreateShader("GlobalLighting.frag", GL_FRAGMENT_SHADER);
	glBindAttribLocation(scene.GlobalLightingShader.program, 0, "vertex");
	glBindAttribLocation(scene.GlobalLightingShader.program, 1, "vertexNormal");
	glBindAttribLocation(scene.GlobalLightingShader.program, 2, "vertexTexture");
	glBindAttribLocation(scene.GlobalLightingShader.program, 3, "vertexTangent");
	scene.GlobalLightingShader.LinkProgram();

    // Read in the needed texture maps
    try {
        glimg::ImageSet* img;

        img = glimg::loaders::stb::LoadFromFile("6670-diffuse.jpg");//6670-diffuse
        scene.groundColor = glimg::CreateTexture(img, 0);
        glBindTexture(GL_TEXTURE_2D, scene.groundColor);

		//img = glimg::loaders::stb::LoadFromFile("ground.jpg");//6670-diffuse
		//scene.skyColor = glimg::CreateTexture(img, 0);
		//glBindTexture(GL_TEXTURE_2D, scene.skyColor);

		img = glimg::loaders::stb::LoadFromFile("6670-normal.jpg");//6670-diffuse
		scene.groundNormal = glimg::CreateTexture(img, 0);
		glBindTexture(GL_TEXTURE_2D, scene.groundNormal);
		

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0); }
    catch (glimg::loaders::stb::UnableToLoadException e) {
        printf("%s\n", e.what());
        exit(-1); }

    CHECKERROR;
}

////////////////////////////////////////////////////////////////////////
// A small helper function to draw a model after settings its lighting
// and modeling parmaeters.
//void DrawSkyDome(Scene scene, const int program, Model* m, mat4x4& ModelTr)
//{
//	int loc = glGetUniformLocation(program, "ModelMatrix");
//	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ModelTr));
//
//	mat4x4 NRM = inverseTranspose(ModelTr);
//	loc = glGetUniformLocation(program, "NormalMatrix");
//	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(NRM));
//
//	loc = glGetUniformLocation(program, "phongDiffuse");
//	glUniform3fv(loc, 1, &m->diffuseColor[0]);
//
//	loc = glGetUniformLocation(program, "phongSpecular");
//	glUniform3fv(loc, 1, &m->specularColor[0]);
//
//	loc = glGetUniformLocation(program, "phongShininess");
//	glUniform1f(loc, m->shininess);
//
//	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, scene.skyColor);
//	loc = glGetUniformLocation(program, "groundColor");
//	glUniform1i(loc, 1);
//
//	//glActiveTexture(GL_TEXTURE2);
//	//glBindTexture(GL_TEXTURE_2D, scene.groundColor);
//	//loc = glGetUniformLocation(program, "groundInfo");
//	//glUniform1i(loc, 2);
//
//	loc = glGetUniformLocation(program, "useSkyDome");
//	glUniform1i(loc, 1);
//
//	loc = glGetUniformLocation(program, "useTexture");
//	glUniform1i(loc, 1);
//
//	m->DrawVAO();
//
//	loc = glGetUniformLocation(program, "useTexture");
//	glUniform1i(loc, 0);
//
//	loc = glGetUniformLocation(program, "useSkyDome");
//	glUniform1i(loc, 0);
//}

////////////////////////////////////////////////////////////////////////
// A small helper function to draw a model after settings its lighting
// and modeling parmaeters.
void DrawModel(const int program, Model* m, mat4x4& ModelTr)
{
    int loc = glGetUniformLocation(program, "ModelMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ModelTr));

    mat4x4 NRM = inverseTranspose(ModelTr);
    loc = glGetUniformLocation(program, "NormalMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(NRM));

    loc = glGetUniformLocation(program, "phongDiffuse");
    glUniform3fv(loc, 1, &m->diffuseColor[0]);

    loc = glGetUniformLocation(program, "phongSpecular");
    glUniform3fv(loc, 1, &m->specularColor[0]);

	if (lightPass) {

		loc = glGetUniformLocation(program, "useReflection");
		glUniform1i(loc, 1);


		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, fbo_reflection_top->texture);
		loc = glGetUniformLocation(program, "reflection_top");
		glUniform1i(loc, 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, fbo_reflection_bottom->texture);
		loc = glGetUniformLocation(program, "reflection_bottom");
		glUniform1i(loc, 4);

		
	}

    loc = glGetUniformLocation(program, "phongShininess");
    glUniform1f(loc, m->shininess);

    m->DrawVAO();

	loc = glGetUniformLocation(program, "useReflection");
	glUniform1i(loc, 0);
}

////////////////////////////////////////////////////////////////////////
// A small helper function for DrawScene to draw all the environment
// spheres.
void DrawSpheres(Scene &scene, unsigned int program, mat4x4& ModelTr)
{
	CHECKERROR;
	float t = 1.0;
	float s = 200.0;
	float color[3];

	int loc;

	loc = glGetUniformLocation(program, "phongSpecular");
	glUniform3fv(loc, 1, &scene.spherePolygons->specularColor[0]);

	loc = glGetUniformLocation(program, "phongShininess");
	glUniform1f(loc, scene.spherePolygons->shininess);

	for (int i = 0; i<2 * scene.nSpheres; i += 2) {
		float u = float(i) / (2 * scene.nSpheres);

		for (int j = 0; j <= scene.nSpheres / 2; j += 2) {
			float v = float(j) / (scene.nSpheres);
			HSV2RGB(u, 1.0f - 2.0f*fabs(v - 0.5f), 1.0f, color);

			float s = 3.0f* sin(v*3.14f);
			mat4x4 M1 = rotate(ModelTr, 360.0f*u, 0.0f, 0.0f, 1.0f);
			mat4x4 M2 = rotate(M1, 180.0f*v, 0.0f, 1.0f, 0.0f);
			mat4x4 M3 = translate(M2, 0.0f, 0.0f, 30.0f);
			mat4x4 M4 = scale(M3, s, s, s);
			loc = glGetUniformLocation(program, "ModelMatrix");
			glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(M4));
			loc = glGetUniformLocation(program, "NormalMatrix");
			mat4x4 NRM = inverseTranspose(M4);
			glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(NRM));

			loc = glGetUniformLocation(program, "phongDiffuse");
			glUniform3fv(loc, 1, color);
			scene.spherePolygons->DrawVAO();
		}
	}

	loc = glGetUniformLocation(program, "ModelMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(mat4x4(1.0f)));
	loc = glGetUniformLocation(program, "NormalMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(mat4x4(1.0f)));
	CHECKERROR;
}

void DrawGround(Scene &scene, unsigned int program, mat4x4& ModelTr)
{
    int loc = glGetUniformLocation(program, "phongDiffuse");
    glUniform3fv(loc, 1, &scene.groundPolygons->diffuseColor[0]);

    loc = glGetUniformLocation(program, "phongSpecular");
    glUniform3fv(loc, 1, &scene.groundPolygons->specularColor[0]);

    loc = glGetUniformLocation(program, "phongShininess");
    glUniform1f(loc, scene.groundPolygons->shininess);

	loc = glGetUniformLocation(program, "useTexture");
	glUniform1i(loc, 1);


	loc = glGetUniformLocation(program, "useNormalMap");
	glUniform1i(loc, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene.groundNormal);
	loc = glGetUniformLocation(program, "normal_map");
	glUniform1i(loc, 0);


    glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, scene.groundColor);//gbuff->m_textures[0]  scene.groundColor

	loc = glGetUniformLocation(program, "groundColor");
    glUniform1i(loc, 1);

    loc = glGetUniformLocation(program, "ModelMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ModelTr));
    mat4x4 NRM = inverseTranspose(ModelTr);
    loc = glGetUniformLocation(program, "NormalMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(NRM));

    scene.groundPolygons->DrawVAO();
    CHECKERROR;

	loc = glGetUniformLocation(program, "useNormalMap");
	glUniform1i(loc, 0);

    loc = glGetUniformLocation(program, "useTexture");
	glUniform1i(loc, 0);

}

void DrawScreenQuad(Scene &scene, unsigned int program, mat4x4& ModelTr)
{


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuff->m_textures[1]);
	GLuint loc = glGetUniformLocation(program, "groundColor");
	glUniform1i(loc, 0);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, scene.screenQuad->vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 6); // Starting from vertex 0; 3 vertices total -> 1 triangle

	glDisableVertexAttribArray(0);
}

void DrawSun(Scene &scene, unsigned int program, mat4x4& ModelTr)
{
    int loc = glGetUniformLocation(program, "ModelMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ModelTr));

    scene.spherePolygons->DrawVAO();
    CHECKERROR;
}

////////////////////////////////////////////////////////////////////////
// Called regularly to update the rotation of the surrounding sphere
// environment.  Set to rotate once every two minutes.
float atime = 0.0;
void animate(int value)
{
    atime = 360.0f*glutGet(GLUT_ELAPSED_TIME)/120000.0f;
    glutPostRedisplay();
}

////////////////////////////////////////////////////////////////////////
// Procedure DrawScene is called whenever the scene needs to be drawn.
void DrawScene(Scene &scene)
{

	BuildScene(scene);
	//ReflectionSceneTop(scene);
	//ReflectionSceneBottom(scene);
	GBufferPass(scene);
	//AmbiantPass(scene);
	GlobalPass(scene);
	return;
    CHECKERROR;

	lightPass = true;
    int loc, program;

	// Calculate the light's position.
	float lPos[4] = {
		scene.lightDist*cos(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*sin(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*cos(scene.lightTilt*rad),
		1.0 };


	mat4x4 SphereModelTr = rotate(atime, 0.0f, 0.0f, 1.0f);
	mat4x4 SunModelTr = translate(lPos[0], lPos[1], lPos[2]);
	mat4x4 skyDomeTr = scale(mat4x4(1), glm::vec3(skyDomeSize, skyDomeSize, skyDomeSize));

	float sx, sy;
	sy = 0.6f*scene.front;
	sx = sy * scene.width / scene.height;

	//////////////////////////pass two
    mat4x4 T1 = translate(scene.translatex, scene.translatey, -scene.zoom);
    mat4x4 T2 = rotate(T1, scene.eyeTilt, 1.0f, 0.0f, 0.0f);
    mat4x4 WorldView = rotate(T2, scene.eyeSpin, 0.0f, 0.0f, 1.0f);
    mat4x4 WorldInv = affineInverse(WorldView);
    mat4x4 WorldProj = frustum(-sx, sx, -sy, sy, scene.front, 10000.0f);
	
	//shadow stuff
	mat4x4 halfScale = scale(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));
	mat4x4 halfTranslate = translate(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));
	
	mat4x4 B = halfTranslate * halfScale;
	//mat4x4 B = halfScale*halfTranslate;
	ShadowMatrix = B * WorldProj * ShadowWorldView;

    glutTimerFunc(100, animate, 1);

    ///////////////////////////////////////////////////////////////////
    // Lighting pass: Draw the scene with lighting being calculated in
    // the lighting shader.
    ///////////////////////////////////////////////////////////////////


    program = scene.lightingShader.program;
    // Set the viewport, and clear the screen
    glViewport(0,0,scene.width, scene.height);
	glClearColor(0.2, 0.2, 0.2, 1);
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);

    // Use lighting pass shader
    scene.lightingShader.Use();

    // Setup the perspective and modelview matrices for normal viewing.
    loc = glGetUniformLocation(program, "ProjectionMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldProj));
    loc = glGetUniformLocation(program, "ViewMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldView));
    loc = glGetUniformLocation(program, "ViewInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldInv));
	//shadow stuff
	loc = glGetUniformLocation(program, "ShadowMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ShadowMatrix));
    CHECKERROR;

	//pass texture using texture2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fbo_shadow->texture);
	loc = glGetUniformLocation(program, "shadowMap");
	glUniform1i(loc, 2);
	//glBindTexture(GL_TEXTURE_2D, 0);

    // Setup the initial model matrix (in gl_ModelViewMatrix)
    loc = glGetUniformLocation(program, "ModelMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
    loc = glGetUniformLocation(program, "NormalMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
    CHECKERROR;

    // Make each texture from earlier passes active in a texture unit, and
    // inform lightingShader.
    loc = glGetUniformLocation(program, "lightAmbient");
    glUniform3fv(loc, 1, ambientColor);
    loc = glGetUniformLocation(program, "lightPos");
    glUniform3fv(loc, 1, lPos);
    loc = glGetUniformLocation(program, "lightValue");
    glUniform3fv(loc, 1, lightColor);

    loc = glGetUniformLocation(program, "mode");
    glUniform1i(loc, scene.mode);

    loc = glGetUniformLocation(program, "WIDTH");
    glUniform1i(loc, scene.width);

    loc = glGetUniformLocation(program, "HEIGHT");
    glUniform1i(loc, scene.height);


    // Draw the scene objects.
    DrawSun(scene, program, SunModelTr);
    if (scene.drawSpheres) DrawSpheres(scene, program, SphereModelTr);
    DrawModel(program, scene.centralPolygons, scene.centralTr);

	if (scene.drawGround) DrawGround(scene, program, Identity);
	//DrawEarth(scene, program, scene.centralPolygons, scene.centralTr);
	//DrawSkyDome(scene, program, scene.skyDome, skyDomeTr);
    CHECKERROR;
	
    // Done with shader program
    scene.lightingShader.Unuse();
	CHECKERROR;

}

void BuildScene(Scene &scene) {

	///////////////////////////////////////////////////////////////////////////////////
	// This is pass one for shadow mapping
	///////////////////////////////////////////////////////////////////////////////////

	CHECKERROR;

	lightPass = false;
	int loc, program;

	//scene.lightSpin += atime / 50.0f;
	//scene.lightTilt += atime / 10.0f;

	// Calculate the light's position.
	float lPos[4] = {
		scene.lightDist*cos(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*sin(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*cos(scene.lightTilt*rad),
		1.0 };


	mat4x4 SphereModelTr = rotate(atime, 0.0f, 0.0f, 1.0f);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);


	float sx, sy;
	sy = 0.6f*scene.front;
	sx = sy * scene.width / scene.height;

	//binding depth buffer
	fbo_shadow->Bind();


	ShadowWorldView = glm::lookAt(glm::vec3(lPos[0], lPos[1], lPos[2]), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
	mat4x4 WorldProj = frustum(-sx, sx, -sy, sy, scene.front, 10000.0f);

	glutTimerFunc(100, animate, 1);

	///////////////////////////////////////////////////////////////////
	// Shadow pass: Draw the scene with depth being calculated in
	// the shadow shader.
	///////////////////////////////////////////////////////////////////

	program = scene.shadowShader.program;

	// Set the viewport, and clear the screen
	glViewport(0, 0, 1024, 1024);
	//glClearColor(0.5, 0.5, 0.5, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use lighting pass shader
	scene.shadowShader.Use();

	// Setup the perspective and modelview matrices for normal viewing.
	loc = glGetUniformLocation(program, "ProjectionMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldProj));
	loc = glGetUniformLocation(program, "ViewMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ShadowWorldView));
	CHECKERROR;

	// Setup the initial model matrix (in gl_ModelViewMatrix)
	loc = glGetUniformLocation(program, "ModelMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));


	// Draw the scene objects.
	//DrawSun(scene, program, SunModelTr);
	if (scene.drawSpheres) DrawSpheres(scene, program, SphereModelTr);
	if (scene.drawGround) DrawGround(scene, program, groundModelTr);
	if (scene.drawTeapot) DrawModel(program, scene.centralPolygons, scene.centralTr);
	///if (scene.drawDragon) DrawModel(program, scene.dragonPolygons, scene.dragonTr);
	//if (scene.drawHappy) DrawModel(program, scene.happyPolygons, scene.happyTr);
	//if (scene.drawBunny) DrawModel(program, scene.bunnyPolygons, scene.bunnyTr);
	//DrawEarth(scene, program, scene.centralPolygons, scene.centralTr);
	CHECKERROR;
	glDisable(GL_CULL_FACE);
	// Done with shader program
	scene.shadowShader.Unuse();
	fbo_shadow->Unbind();
	
	CHECKERROR;

}

void ReflectionSceneTop(Scene &scene) {

	lightPass = false;
	int loc, program;

	// Calculate the light's position.
	float lPos[4] = {
		scene.lightDist*cos(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*sin(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*cos(scene.lightTilt*rad),
		1.0 };


	mat4x4 SphereModelTr = rotate(atime, 0.0f, 0.0f, 1.0f);
	mat4x4 SunModelTr = translate(lPos[0], lPos[1], lPos[2]);
	mat4x4 skyDomeTr = scale(mat4x4(1), glm::vec3(skyDomeSize, skyDomeSize, skyDomeSize));

	float sx, sy;
	sy = 0.6f*scene.front;
	sx = sy * scene.width / scene.height;


	//binding depth buffer
	fbo_reflection_top->Bind();


	//////////////////////////pass two
	//mat4x4 T1 = translate(scene.translatex, scene.translatey, -scene.zoom);
	//mat4x4 T1 = translate(0.0f, 0.0f, 0.0f);
	mat4x4 T1 = translate(scene.translatex, scene.translatey, -scene.zoom);
	mat4x4 T2 = rotate(T1, scene.eyeTilt, 1.0f, 0.0f, 0.0f);
	mat4x4 WorldView = rotate(T2, scene.eyeSpin, 0.0f, 0.0f, 1.0f);

	mat4x4 WorldInv = affineInverse(WorldView);
	mat4x4 WorldProj = frustum(-sx, sx, -sy, sy, scene.front, 10000.0f);

	//shadow stuff
	mat4x4 halfScale = scale(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));
	mat4x4 halfTranslate = translate(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));

	mat4x4 B = halfTranslate * halfScale;
	//mat4x4 B = halfScale*halfTranslate;
	mat4x4 ShadowMatrix = B * WorldProj * ShadowWorldView;

	glutTimerFunc(100, animate, 1);

	///////////////////////////////////////////////////////////////////
	// Lighting pass: Draw the scene with lighting being calculated in
	// the lighting shader.
	///////////////////////////////////////////////////////////////////


	program = scene.reflectionShader.program;
	// Set the viewport, and clear the screen
	//glViewport(0, 0, scene.width, scene.height);
	glViewport(0, 0, 512, 512);
	glClearColor(0.2, 0.2, 0.2, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use lighting pass shader
	scene.reflectionShader.Use();

	// Setup the perspective and modelview matrices for normal viewing.
	loc = glGetUniformLocation(program, "ProjectionMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldProj));
	loc = glGetUniformLocation(program, "ViewMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldView));
	loc = glGetUniformLocation(program, "ViewInverse");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldInv));
	//shadow stuff
	loc = glGetUniformLocation(program, "ShadowMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ShadowMatrix));

	loc = glGetUniformLocation(program, "isTop");
	glUniform1i(loc, 1);


	CHECKERROR;

	//pass texture using texture2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fbo_shadow->texture);
	loc = glGetUniformLocation(program, "shadowMap");
	glUniform1i(loc, 2);
	//glBindTexture(GL_TEXTURE_2D, 0);

	// Setup the initial model matrix (in gl_ModelViewMatrix)
	loc = glGetUniformLocation(program, "ModelMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
	loc = glGetUniformLocation(program, "NormalMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
	CHECKERROR;

	// Make each texture from earlier passes active in a texture unit, and
	// inform lightingShader.
	loc = glGetUniformLocation(program, "lightAmbient");
	glUniform3fv(loc, 1, ambientColor);
	loc = glGetUniformLocation(program, "lightPos");
	glUniform3fv(loc, 1, lPos);
	loc = glGetUniformLocation(program, "lightValue");
	glUniform3fv(loc, 1, lightColor);

	loc = glGetUniformLocation(program, "mode");
	glUniform1i(loc, scene.mode);

	loc = glGetUniformLocation(program, "WIDTH");
	glUniform1i(loc, scene.width);

	loc = glGetUniformLocation(program, "HEIGHT");
	glUniform1i(loc, scene.height);


	// Draw the scene objects.
	DrawSun(scene, program, SunModelTr);
	if (scene.drawSpheres) DrawSpheres(scene, program, SphereModelTr);
	//DrawModel(program, scene.centralPolygons, scene.centralTr);
	if (scene.drawGround) DrawGround(scene, program, Identity);
	//DrawEarth(scene, program, scene.centralPolygons, scene.centralTr);
	//DrawSkyDome(scene, program, scene.skyDome, skyDomeTr);
	CHECKERROR;

	// Done with shader program
	scene.reflectionShader.Unuse();
	fbo_reflection_top->Unbind();
	CHECKERROR;
}

void ReflectionSceneBottom(Scene &scene) {

	lightPass = false;
	int loc, program;

	// Calculate the light's position.
	float lPos[4] = {
		scene.lightDist*cos(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*sin(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*cos(scene.lightTilt*rad),
		1.0 };


	mat4x4 SphereModelTr = rotate(atime, 0.0f, 0.0f, 1.0f);
	mat4x4 SunModelTr = translate(lPos[0], lPos[1], lPos[2]);
	mat4x4 skyDomeTr = scale(mat4x4(1), glm::vec3(skyDomeSize, skyDomeSize, skyDomeSize));

	float sx, sy;
	sy = 0.6f*scene.front;
	sx = sy * scene.width / scene.height;


	//binding depth buffer
	fbo_reflection_bottom->Bind();


	//////////////////////////pass two
	mat4x4 T1 = translate(scene.translatex, scene.translatey, -scene.zoom);
	mat4x4 T2 = rotate(T1, scene.eyeTilt, 1.0f, 0.0f, 0.0f);
	mat4x4 WorldView = rotate(T2, scene.eyeSpin, 0.0f, 0.0f, 1.0f);

	mat4x4 WorldInv = affineInverse(WorldView);
	mat4x4 WorldProj = frustum(-sx, sx, -sy, sy, scene.front, 10000.0f);

	//shadow stuff
	mat4x4 halfScale = scale(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));
	mat4x4 halfTranslate = translate(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));

	mat4x4 B = halfTranslate * halfScale;
	//mat4x4 B = halfScale*halfTranslate;
	mat4x4 ShadowMatrix = B * WorldProj * ShadowWorldView;

	glutTimerFunc(100, animate, 1);

	///////////////////////////////////////////////////////////////////
	// Lighting pass: Draw the scene with lighting being calculated in
	// the lighting shader.
	///////////////////////////////////////////////////////////////////


	program = scene.reflectionShader.program;
	// Set the viewport, and clear the screen
	//glViewport(0, 0, scene.width, scene.height);
	glViewport(0, 0, 512, 512);
	glClearColor(0.2, 0.2, 0.2, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use lighting pass shader
	scene.reflectionShader.Use();

	// Setup the perspective and modelview matrices for normal viewing.
	loc = glGetUniformLocation(program, "ProjectionMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldProj));
	loc = glGetUniformLocation(program, "ViewMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldView));
	loc = glGetUniformLocation(program, "ViewInverse");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldInv));
	//shadow stuff
	loc = glGetUniformLocation(program, "ShadowMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ShadowMatrix));

	loc = glGetUniformLocation(program, "isTop");
	glUniform1i(loc, 0);

	CHECKERROR;

	//pass texture using texture2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fbo_shadow->texture);
	loc = glGetUniformLocation(program, "shadowMap");
	glUniform1i(loc, 2);
	//glBindTexture(GL_TEXTURE_2D, 0);

	// Setup the initial model matrix (in gl_ModelViewMatrix)
	loc = glGetUniformLocation(program, "ModelMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
	loc = glGetUniformLocation(program, "NormalMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
	CHECKERROR;

	// Make each texture from earlier passes active in a texture unit, and
	// inform lightingShader.
	loc = glGetUniformLocation(program, "lightAmbient");
	glUniform3fv(loc, 1, ambientColor);
	loc = glGetUniformLocation(program, "lightPos");
	glUniform3fv(loc, 1, lPos);
	loc = glGetUniformLocation(program, "lightValue");
	glUniform3fv(loc, 1, lightColor);

	loc = glGetUniformLocation(program, "mode");
	glUniform1i(loc, scene.mode);

	loc = glGetUniformLocation(program, "WIDTH");
	glUniform1i(loc, scene.width);

	loc = glGetUniformLocation(program, "HEIGHT");
	glUniform1i(loc, scene.height);


	// Draw the scene objects.
	DrawSun(scene, program, SunModelTr);
	if (scene.drawSpheres) DrawSpheres(scene, program, SphereModelTr);
	//DrawSkyDome(scene, program, scene.skyDome, skyDomeTr);
	//DrawModel(program, scene.centralPolygons, scene.centralTr);
	//if (scene.drawGround) DrawGround(scene, program, Identity);
	//DrawEarth(scene, program, scene.centralPolygons, scene.centralTr);
	CHECKERROR;

	// Done with shader program
	scene.reflectionShader.Unuse();
	fbo_reflection_bottom->Unbind();
	CHECKERROR;
}

void GBufferPass(Scene &scene)
{
	///////////////////////////////////////////////////////////////////////////////////
	// This is pass one for shadow mapping
	///////////////////////////////////////////////////////////////////////////////////

	CHECKERROR;

	lightPass = false;
	int loc, program;

	// Calculate the light's position.
	float lPos[4] = {
		scene.lightDist*cos(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*sin(scene.lightSpin*rad)*sin(scene.lightTilt*rad),
		scene.lightDist*cos(scene.lightTilt*rad),
		1.0 };


	mat4x4 SphereModelTr = rotate(atime, 0.0f, 0.0f, 1.0f);
	mat4x4 skyDomeTr = scale(mat4x4(1), glm::vec3(skyDomeSize, skyDomeSize, skyDomeSize));
	mat4x4 groundTr = scale(mat4x4(1), glm::vec3(5, 5, 1));
	mat4x4 ModelTr = rotate(90.0f, 1.0f, 0.0f, 0.0f);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	float sx, sy;
	sy = 0.6f*scene.front;
	sx = sy * scene.width / scene.height;


	mat4x4 T1 = translate(scene.translatex, scene.translatey, -scene.zoom);
	mat4x4 T2 = rotate(T1, scene.eyeTilt, 1.0f, 0.0f, 0.0f);
	mat4x4 WorldView = rotate(T2, scene.eyeSpin, 0.0f, 0.0f, 1.0f);
	mat4x4 WorldInv = affineInverse(WorldView);
	mat4x4 WorldProj = frustum(-sx, sx, -sy, sy, scene.front, 10000.0f);

	//shadow stuff
	mat4x4 halfScale = scale(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));
	mat4x4 halfTranslate = translate(glm::vec3(1 / 2.0, 1 / 2.0, 1 / 2.0));

	mat4x4 B = halfTranslate * halfScale;
	//mat4x4 B = halfScale*halfTranslate;
	ShadowMatrix = B * WorldProj * ShadowWorldView;

	glutTimerFunc(100, animate, 1);

	///////////////////////////////////////////////////////////////////
	// Shadow pass: Draw the scene with depth being calculated in
	// the shadow shader.
	///////////////////////////////////////////////////////////////////
	//binding depth buffer
	gbuff->Bind();
	program = scene.GBufferShader.program;

	// Set the viewport, and clear the screen
	glViewport(0, 0, 1024, 1024);
	//glClearColor(0.5, 0.5, 0.5, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use lighting pass shader
	scene.GBufferShader.Use();

	ShadowWorldView = glm::lookAt(glm::vec3(lPos[0], lPos[1], lPos[2]), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
	// Setup the perspective and modelview matrices for normal viewing.
	loc = glGetUniformLocation(program, "ProjectionMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldProj));
	loc = glGetUniformLocation(program, "ViewMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldView));
	loc = glGetUniformLocation(program, "ViewInverse");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(WorldInv));

	//shadow stuff
	loc = glGetUniformLocation(program, "ShadowMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ShadowMatrix));

	// Setup the initial model matrix (in gl_ModelViewMatrix)
	loc = glGetUniformLocation(program, "ModelMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
	loc = glGetUniformLocation(program, "NormalMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(Identity));
	loc = glGetUniformLocation(program, "lightAmbient");
	glUniform3fv(loc, 1, ambientColor);
	loc = glGetUniformLocation(program, "lightPos");
	glUniform3fv(loc, 1, lPos);
	CHECKERROR;
	// Draw the scene objects.
	//DrawSun(scene, program, SunModelTr);
	if (scene.drawSpheres) DrawSpheres(scene, program, SphereModelTr);
	glDisable(GL_CULL_FACE);
	if (scene.drawGround) DrawGround(scene, program, groundModelTr);
	if (scene.drawTeapot) DrawModel(program, scene.centralPolygons, scene.centralTr);
	//if (scene.drawDragon) DrawModel(program, scene.dragonPolygons, scene.dragonTr);
	//if (scene.drawHappy) DrawModel(program, scene.happyPolygons, scene.happyTr);
	//if (scene.drawBunny) DrawModel(program, scene.bunnyPolygons, scene.bunnyTr);
	//DrawSkyDome(scene, program, scene.skyDome, skyDomeTr);
	//DrawEarth(scene, program, scene.centralPolygons, scene.centralTr);
	CHECKERROR;
	// Done with shader program
	scene.GBufferShader.Unuse();
	gbuff->Unbind();

	CHECKERROR;
}

void AmbiantPass(Scene &scene)
{
	///////////////////////////////////////////////////////////////////////////////////
	// This is pass one for shadow mapping
	///////////////////////////////////////////////////////////////////////////////////
	CHECKERROR;

	lightPass = true;
	int program;

	glDisable(GL_CULL_FACE);
	///////////////////////////////////////////////////////////////////
	// Shadow pass: Draw the scene with depth being calculated in
	// the shadow shader.
	///////////////////////////////////////////////////////////////////
	//binding depth buffer
	//gbuff->Bind();
	program = scene.AmbiantShader.program;
	// Use lighting pass shader
	scene.AmbiantShader.Use();

	// Set the viewport, and clear the screen
	glViewport(0, 0, scene.width, scene.height);
	glClearColor(0.5, 0.5, 0.5, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw the scene objects.
	DrawScreenQuad(scene, program, Identity);

	// Done with shader program
	scene.AmbiantShader.Unuse();
	gbuff->Unbind();

	CHECKERROR;
}

void GlobalPass(Scene &scene)
{
	///////////////////////////////////////////////////////////////////////////////////
	// This is pass one for shadow mapping
	///////////////////////////////////////////////////////////////////////////////////
	CHECKERROR;

	lightPass = true;
	int program;

	//glDisable(GL_CULL_FACE);
	//glDisable(GL_DEPTH_TEST);
	program = scene.GlobalLightingShader.program;
	// Use lighting pass shader
	scene.GlobalLightingShader.Use();

	// Set the viewport, and clear the screen
	glViewport(0, 0, scene.width, scene.height);
	glClearColor(0.1, 0.1, 0.1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuff->m_textures[GBUFFER_TEXTURE_TYPE_POSITION]);
	GLuint loc = glGetUniformLocation(program, "GpositionMap");
	glUniform1i(loc, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gbuff->m_textures[GBUFFER_TEXTURE_TYPE_NORMAL]);
	loc = glGetUniformLocation(program, "GnormalMap");
	glUniform1i(loc, 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gbuff->m_textures[GBUFFER_TEXTURE_TYPE_TEXCOORD]);
	loc = glGetUniformLocation(program, "GuvMap");
	glUniform1i(loc, 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, gbuff->m_textures[GBUFFER_TEXTURE_TYPE_EYEVEC]);
	loc = glGetUniformLocation(program, "EyeVecMap");
	glUniform1i(loc, 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, gbuff->m_textures[GBUFFER_TEXTURE_TYPE_LIGHTVEC]);
	loc = glGetUniformLocation(program, "LightVecMap");
	glUniform1i(loc, 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, gbuff->m_textures[GBUFFER_TEXTURE_TYPE_SHADOW_CORD]);
	loc = glGetUniformLocation(program, "ShadowCordMap");
	glUniform1i(loc, 6);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, fbo_shadow->texture);
	loc = glGetUniformLocation(program, "depthMap");
	glUniform1i(loc, 7);

	loc = glGetUniformLocation(program, "RenderMode");
	glUniform1i(loc, scene.RenderMode);

	loc = glGetUniformLocation(program, "Time");
	glUniform1f(loc, atime);


	// Draw the scene objects.
	DrawScreenQuad(scene, program, Identity);

	// Done with shader program
	scene.GlobalLightingShader.Unuse();

	CHECKERROR;
}