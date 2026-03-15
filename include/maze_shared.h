#ifndef MAZE_SHARED_H
#define MAZE_SHARED_H

#include <windows.h>
#include <GL/glut.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#define WIDTH 22
#define HEIGHT 14
#define TILE 1.0
#define START_ROOM_HALF_SIZE 2
#define WALL_HEIGHT 2.53f
#define PLAYER_HEIGHT 0.55f
#define PLAYER_COLLISION_RADIUS 0.244f
#define MOVE_SPEED 0.01225f
#define MOUSE_SENSITIVITY 0.002625f
#define MAX_PITCH 1.2f
#define SCENE_BRIGHTNESS 0.02f
#define SKYBOX_SIZE 30.0f
#define SKYBOX_HEIGHT 2.0f
#define SKYBOX_Y_OFFSET -2.0f
#define SKY_SIDE_V_MIN 0.0f
#define WALK_BOB_AMPLITUDE 0.028f
#define WALK_BOB_FREQ 11.0f

extern float playerX;
extern float playerY;
extern float playerAngle;
extern float playerPitch;
extern float cameraBobOffset;
extern float walkBobPhase;
extern int windowWidth;
extern int windowHeight;
extern int mouseInitialized;
extern int ignoreMouseWarp;
extern bool keyStates[256];
extern bool endReached;
extern bool flashlightEnabled;
extern float flashlightIntensity;

extern GLuint wallTexture;
extern GLuint floorTexture;
extern GLuint skyTexture;
extern GLuint gateTexture;
extern GLuint skeletonTexture;

extern int endTileX;
extern int endTileY;
extern int maze[HEIGHT][WIDTH];

float clampf(float value, float minValue, float maxValue);
int isWall(float x, float y);
int canMoveTo(float x, float y);

#endif