///////////////////////////////////////////////////////////////////////
// Provides the framework for graphics projects.  Most of this small
// file contains the GLUT calls needed to open a window and hook up
// various callbacks for mouse/keyboard interaction and screen resizes
// and redisplays.
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
    // Includes for Windows
    #include <windows.h>
    #include <cstdlib>
    #include <fstream>
    #include <limits>
    #include <crtdbg.h>
#else
    // Includes for Linux
    #include <algorithm>
    #define sprintf_s sprintf
    #include <fstream>
#endif

#include <glload/gl_3_3.h>
#include <glload/gl_load.hpp>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtx/std_based_type.hpp>

#include <vector>
#include "math.h"
#include "shader.h"
#include "fbo.h"
#include "scene.h"
#include "AntTweakBar.h"

using namespace glm;

static mat4 Identity(1.0f);

TwBar *bar;

Scene scene;

// Some globals used for mouse handling.
int mouseX, mouseY;
bool leftDown = false;
bool middleDown = false;
bool rightDown = false;
bool shifted;

////////////////////////////////////////////////////////////////////////
// Called by GLUT when the scene needs to be redrawn.
void ReDraw()
{
    DrawScene(scene);
    TwDraw();
    glutSwapBuffers();
}

////////////////////////////////////////////////////////////////////////
// Called by GLUT when the window size is changed.
void ReshapeWindow(int w, int h)
{
    if (w && h) {
        glViewport(0, 0, w, h); }
    TwWindowSize(w,h);
    scene.width = w;
    scene.height = h;

    // Force a redraw
    glutPostRedisplay();
}

////////////////////////////////////////////////////////////////////////
// Called by GLut for keyboard actions.
void KeyboardDown(unsigned char key, int x, int y)
{
    if (TwEventKeyboardGLUT(key, x, y)) return;

    switch(key) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        scene.mode = key-'0';
        glutPostRedisplay();
        break;

    case 27:                    // Escape key
    case 'q':
        exit(0);
    }
}

void KeyboardUp(unsigned char key, int x, int y)
{
}

////////////////////////////////////////////////////////////////////////
// Called by GLut when a mouse button changes state.
void MouseButton(int button, int state, int x, int y)
{
    if (TwEventMouseButtonGLUT(button, state, x, y)) return;

    shifted = glutGetModifiers() && GLUT_ACTIVE_SHIFT;

    if (button == GLUT_LEFT_BUTTON)
        leftDown = (state == GLUT_DOWN);

    else if (button == GLUT_MIDDLE_BUTTON)
        middleDown = (state == GLUT_DOWN);

    else if (button == GLUT_RIGHT_BUTTON)
        rightDown = (state == GLUT_DOWN);

    else if (button%8 == 3 && shifted)
        scene.lightDist = pow(scene.lightDist, 1.0f/1.02f);

    else if (button%8 == 3)
        scene.zoom = pow(scene.zoom, 1.0f/1.02f);

    else if (button%8 == 4 && shifted)
        scene.lightDist = pow(scene.lightDist, 1.02f);

    else if (button%8 == 4)
        scene.zoom = pow(scene.zoom, 1.02f);

    mouseX = x;
    mouseY = y;

    glutPostRedisplay();

}

////////////////////////////////////////////////////////////////////////
// Called by GLut when a mouse moves (while a button is down)
void MouseMotion(int x, int y)
{
    if (TwEventMouseMotionGLUT(x,y)) return;

    int dx = x-mouseX;
    int dy = y-mouseY;

    if (leftDown && shifted) {
        scene.lightSpin += dx/3.0f;
        scene.lightTilt -= dy/3.0f; }

    else if (leftDown) {
        scene.eyeSpin += dx/2.0f;
        scene.eyeTilt += dy/2.0f; }


    if (middleDown && shifted) {
        scene.lightDist = pow(scene.lightDist, 1.0f-dy/200.0f);  }

    else if (middleDown) {
        scene.zoom += dy/10.0f; }


    if (rightDown && shifted) {
        /* nothing */ }
    else if (rightDown) {
        scene.translatex += dx/20.0f;
        scene.translatey -= dy/20.0f; }

    // Record this position
    mouseX = x;
    mouseY = y;

    // Force a redraw
    glutPostRedisplay();

}

////////////////////////////////////////////////////////////////////////
// Functions called by AntTweakBar
void Quit(void *clientData)
{
    TwTerminate();
    glutLeaveMainLoop();
}

void ToggleGround(void *clientData)
{
    scene.drawGround = !scene.drawGround;
}

void ToggleSpheres(void *clientData)
{
    scene.drawSpheres = !scene.drawSpheres;
}

void TW_CALL SetModel(const void *value, void *clientData)
{
    scene.centralModel = *(int*)value; // AntTweakBar forces this cast.
    delete scene.centralPolygons;
    scene.centralPolygons = NULL;

    if (scene.centralModel==0) {
        scene.centralPolygons =  new Teapot(12);    
        float s = 3.0/scene.centralPolygons->size;
        scene.centralTr =
            scale(Identity, s,s,s)
            *translate(-scene.centralPolygons->center); }

    else if (scene.centralModel==1) {
        scene.centralPolygons = new Ply("bunny.ply");
        float s = 3.0/scene.centralPolygons->size;
        scene.centralTr =
            rotate(Identity, 180.0f, 0.0f, 0.0f, 180.0f)
            *rotate(Identity, 90.0f, 1.0f, 0.0f, 0.0f)
            *scale(Identity, s,s,s)
            *translate(-scene.centralPolygons->center); }

    else if (scene.centralModel==2) {
        scene.centralPolygons = new Ply("dragon.ply");
        float s = 3.0/scene.centralPolygons->size;
        scene.centralTr =
            rotate(Identity, 180.0f, 0.0f, 0.0f, 180.0f)
            *rotate(Identity, 90.0f, 1.0f, 0.0f, 0.0f)
            *scale(Identity, s,s,s)
            *translate(-scene.centralPolygons->center); }

    else {       // Fallback model
        scene.centralPolygons = new Sphere(32);
        scene.centralTr = Identity; }
}

void TW_CALL GetModel(void *value, void *clientData)
{
    *(int*)value = scene.centralModel;
}

////////////////////////////////////////////////////////////////////////
// Do the OpenGL/GLut setup and then enter the interactive loop.
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitContextVersion (3, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);


    glutInitWindowSize(750,750);
    glutCreateWindow("Class Framework");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    glload::LoadFunctions();

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Rendered by: %s\n", glGetString(GL_RENDERER));
	fflush(stdout);

    glutIgnoreKeyRepeat(true);

    TwInit(TW_OPENGL, NULL);
    glutDisplayFunc(&ReDraw);
    glutReshapeFunc(&ReshapeWindow);

    glutKeyboardFunc(&KeyboardDown);
    glutKeyboardUpFunc(&KeyboardUp);

    glutMouseFunc(&MouseButton);
    glutMotionFunc(&MouseMotion);
    glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
    glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
    TwGLUTModifiersFunc((int(TW_CALL*)())glutGetModifiers);

    bar = TwNewBar("Tweaks");
    TwDefine(" Tweaks size='200 300' ");
    TwAddButton(bar, "quit", (TwButtonCallback)Quit, NULL, " label='Quit' key=q ");

    TwAddVarCB(bar, "centralModel", TwDefineEnum("CentralModel", NULL, 0),
               SetModel, GetModel, NULL,
               " enum='0 {Teapot}, 1 {Bunny}, 2 {Dragon}, 3 {Sphere}' ");
    TwAddButton(bar, "Spheres", (TwButtonCallback)ToggleSpheres, NULL, " label='Spheres' ");
    TwAddButton(bar, "Ground", (TwButtonCallback)ToggleGround, NULL, " label='Ground' ");

    InitializeScene(scene);

    // This function enters the event loop.
    glutMainLoop();
}
