#include <math.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "game.h"
#include "model.h"
#include "render.h"

SDL_Window* gWindow = NULL;
SDL_GLContext gGLContext = NULL;
int gRunning = 1;

float playerX = 4.5f;
float playerY = 5.5f;
float playerAngle = 0.0f;
float playerPitch = 0.0f;
float cameraBobOffset = 0.0f;
float walkBobPhase = 0.0f;
float tableX = 3.2f;
float tableZ = 5.5f;
const float tableBlockHalfX = 0.95f;
const float tableBlockHalfZ = 0.65f;
int windowWidth = 1000;
int windowHeight = 700;
bool keyStates[256] = { false };
bool showHelp = false;

GLuint wallTexture = 0;
GLuint floorTexture = 0;
GLuint skyTexture = 0;
GLuint tableTexture = 0;
Model gTableModel = {0};
float flashlightIntensity = SCENE_BRIGHTNESS;

int isWall(float x, float y)
{
    int gridX = (int)floorf(x);
    int gridY = (int)floorf(y);

    if (x < 0.01f || x >= WIDTH - 0.01f || y < 0.01f || y >= HEIGHT - 0.01f)
    {
        return 1;
    }

    if (gridX < 0 || gridX >= WIDTH || gridY < 0 || gridY >= HEIGHT)
    {
        return 1;
    }

    return maze[gridY][gridX] == 1;
}

int canMoveTo(float x, float y)
{
    float dx;
    float dz;
    float expandedHalfX;
    float expandedHalfZ;
    const float radius = PLAYER_COLLISION_RADIUS;

    if (isWall(x - radius, y - radius)
        || isWall(x + radius, y - radius)
        || isWall(x - radius, y + radius)
        || isWall(x + radius, y + radius))
    {
        return 0;
    }

    dx = x - tableX;
    dz = y - tableZ;
    expandedHalfX = tableBlockHalfX + radius;
    expandedHalfZ = tableBlockHalfZ + radius;
    if (fabsf(dx) < expandedHalfX && fabsf(dz) < expandedHalfZ)
    {
        return 0;
    }

    return 1;
}

float clampf(float value, float minValue, float maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

void keyboard(int key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == 27)
    {
        gRunning = 0;
        return;
    }

    if (key == SDLK_F1 || key == 315)
    {
        showHelp = !showHelp;
    }

    if (key == SDLK_UP)
    {
        flashlightIntensity += 0.2f;
        if (flashlightIntensity > 1.0f)
        {
            flashlightIntensity = 1.0f;
        }
        printf("Fenyero: %.1f\n", flashlightIntensity);
    }
    if (key == SDLK_DOWN)
    {
        flashlightIntensity -= 0.2f;
        if (flashlightIntensity < 0.2f)
        {
            flashlightIntensity = 0.2f;
        }
        printf("Fenyero: %.1f\n", flashlightIntensity);
    }

    if (key >= 0 && key < 256)
    {
        keyStates[(unsigned char)key] = true;
    }
}

void keyboardUp(int key, int x, int y)
{
    (void)x;
    (void)y;

    if (key >= 0 && key < 256)
    {
        keyStates[(unsigned char)key] = false;
    }
}

void mouseMotion(int dx, int dy)
{
    int deltaX = dx;
    int deltaY = dy;

    if (deltaX != 0 || deltaY != 0)
    {
        playerAngle += deltaX * MOUSE_SENSITIVITY;
        playerPitch -= deltaY * MOUSE_SENSITIVITY;

        if (playerPitch > MAX_PITCH)
        {
            playerPitch = MAX_PITCH;
        }
        if (playerPitch < -MAX_PITCH)
        {
            playerPitch = -MAX_PITCH;
        }
    }
}

void update(void)
{
    float dirX = cosf(playerAngle);
    float dirY = sinf(playerAngle);
    float sideX = dirY;
    float sideY = -dirX;
    float moveX = 0.0f;
    float moveY = 0.0f;
    float newX;
    float newY;
    float oldX;
    float oldY;
    float movedDistance;
    static int lastTime = 0;
    int currentTime;
    float deltaTime;

    currentTime = (int)SDL_GetTicks();
    if (lastTime == 0)
    {
        lastTime = currentTime;
    }
    deltaTime = (currentTime - lastTime) / 1000.0f;

    if (keyStates['w'] || keyStates['W'])
    {
        moveX += dirX * MOVE_SPEED;
        moveY += dirY * MOVE_SPEED;
    }
    if (keyStates['s'] || keyStates['S'])
    {
        moveX -= dirX * MOVE_SPEED;
        moveY -= dirY * MOVE_SPEED;
    }
    if (keyStates['a'] || keyStates['A'])
    {
        moveX += sideX * MOVE_SPEED;
        moveY += sideY * MOVE_SPEED;
    }
    if (keyStates['d'] || keyStates['D'])
    {
        moveX -= sideX * MOVE_SPEED;
        moveY -= sideY * MOVE_SPEED;
    }

    oldX = playerX;
    oldY = playerY;

    newX = playerX + moveX;
    newY = playerY + moveY;

    if (canMoveTo(newX, playerY))
    {
        playerX = newX;
    }
    if (canMoveTo(playerX, newY))
    {
        playerY = newY;
    }

    movedDistance = sqrtf((playerX - oldX) * (playerX - oldX) + (playerY - oldY) * (playerY - oldY));
    if (deltaTime > 0.0f && movedDistance > 0.00001f)
    {
        float moveFactor = clampf((movedDistance / deltaTime) / MOVE_SPEED, 0.0f, 1.0f);
        walkBobPhase += deltaTime * WALK_BOB_FREQ * (0.6f + 0.4f * moveFactor);
        if (walkBobPhase > 2.0f * (float)M_PI)
        {
            walkBobPhase -= 2.0f * (float)M_PI;
        }
        cameraBobOffset = sinf(walkBobPhase) * WALK_BOB_AMPLITUDE * moveFactor;
    }
    else
    {
        float settle = clampf(deltaTime * 9.0f, 0.0f, 1.0f);
        cameraBobOffset += (0.0f - cameraBobOffset) * settle;
    }

    lastTime = currentTime;
}

void init(void)
{
    GLfloat globalAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};

    playerX = (float)startTileX + 0.5f;
    playerY = (float)startTileY + 0.5f;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_DIFFUSE);

    wallTexture = loadTexture(WALL_TEXTURE_PATH, 0);
    floorTexture = loadTexture(FLOOR_TEXTURE_PATH, 1);
    skyTexture = loadTexture(SKY_TEXTURE_PATH, 0);

    if (!loadOBJ("assets/models/picnic_table.obj", &gTableModel))
    {
        if (!loadOBJ("assets/models/Picnic_Table.obj", &gTableModel))
        {
            loadOBJ("assets/models/Picnic Table.obj", &gTableModel);
        }
    }

    tableTexture = loadTexture("assets/models/PicnicTable_MTL_BaseColor.png", 0);

    initBitmapFont();
}