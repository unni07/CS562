////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw the (really) simple scene, including:
//   * Geometry (in a display list)
//   * Light parameters
//   * Material properties
//   * Viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them should be used to draw the scene.

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using namespace glm;

#include "models.h"

class Scene
{
public:
    // Some user controllable parameters
    int mode;  // Communicated to the shaders as "mode".  Keys '0'-'9'
    int nSpheres;
    bool drawSpheres;
    bool drawGround;
	bool drawTeapot;
	bool drawDragon;
	bool drawHappy;
	bool drawBunny;

    int centralType;
    int centralModel;
    mat4 dragonTr;
	mat4 happyTr;
	mat4 bunnyTr;
	mat4 horseTr;
	mat4 centralTr;

    // Viewing transformation parameters;  Mouse buttons 1-3
    float front;
    float eyeSpin;
    float eyeTilt;
    float translatex;
    float translatey;
    float zoom;

    // Light position parameters;  Mouse buttons SHIFT 1-3
    float lightSpin;
    float lightTilt;
    vec3 lightDir;
    float lightDist;

    // Viewport
    int width, height;

    // Shader programs
    ShaderProgram lightingShader;
	ShaderProgram shadowShader;
	ShaderProgram reflectionShader;
	ShaderProgram GBufferShader;
	ShaderProgram AmbiantShader;
	ShaderProgram GlobalLightingShader;

    // The polygon models
    Model* centralPolygons;
	Model* dragonPolygons;
	Model* happyPolygons;
	Model* bunnyPolygons;
	Model* horsePolygons;
    Model* spherePolygons;
    Model* groundPolygons;
	Model* skyDome;
	ScreenQuad* screenQuad;

    // Texture
    int groundColor;
	int groundNormal;
	int skyColor;
	int RenderMode;
};

void InitializeScene(Scene &scene);
void BuildScene(Scene &scene);
void GBufferPass(Scene &scene);
void GlobalPass(Scene &scene);
void AmbiantPass(Scene &scene);
void ReflectionSceneTop(Scene &scene);
void ReflectionSceneBottom(Scene &scene);
void DrawScene(Scene &scene);
