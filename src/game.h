#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "config.h"

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float u, v;
} Vec2;

typedef struct {
    int v[3];
    int vt[3];
    int vn[3];
} Face;

typedef struct {
    Vec3* vertices;
    Vec2* texcoords;
    Vec3* normals;
    Face* faces;
    int vertexCount;
    int texcoordCount;
    int normalCount;
    int faceCount;
    GLuint displayList;
} Model;

extern SDL_Window* gWindow;
extern SDL_GLContext gGLContext;
extern int gRunning;

extern float playerX;
extern float playerY;
extern float playerAngle;
extern float playerPitch;
extern float cameraBobOffset;
extern float walkBobPhase;
extern float tableX;
extern float tableZ;
extern const float tableBlockHalfX;
extern const float tableBlockHalfZ;
extern int windowWidth;
extern int windowHeight;
extern bool keyStates[256];
extern bool showHelp;

extern GLuint wallTexture;
extern GLuint floorTexture;
extern GLuint skyTexture;
extern GLuint tableTexture;
extern Model gTableModel;
extern float flashlightIntensity;

int isWall(float x, float y);
int canMoveTo(float x, float y);
float clampf(float value, float minValue, float maxValue);

void keyboard(int key, int x, int y);
void keyboardUp(int key, int x, int y);
void mouseMotion(int dx, int dy);
void update(void);
void init(void);

#endif