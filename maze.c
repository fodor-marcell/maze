#include <GL/glut.h>
#include <SOIL/SOIL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif

#define WIDTH 22
#define HEIGHT 14
#define TILE 1.0
#define WALL_HEIGHT 4.0f
#define PLAYER_HEIGHT 0.55f
#define MOVE_SPEED 0.0175f
#define MOUSE_SENSITIVITY 0.002625f
#define MAX_PITCH 1.2f
#define SCENE_BRIGHTNESS 0.06f
#define SKYBOX_SIZE 30.0f
#define SKYBOX_HEIGHT 2.0f
#define SKYBOX_Y_OFFSET -2.0f
#define SKY_SIDE_V_MIN 0.0f
#define END_TILE_X 14
#define END_TILE_Y 10

float playerX = 4.5f;
float playerY = 5.5f;
float playerAngle = 0.0f;
float playerPitch = 0.0f;
int windowWidth = 1000;
int windowHeight = 700;
int mouseInitialized = 0;
int ignoreMouseWarp = 0;
bool keyStates[256] = { false };
bool endReached = false;
bool flashlightEnabled = true;
GLuint wallTexture = 0;
GLuint floorTexture = 0;
GLuint skyTexture = 0;

const char* WALL_TEXTURE_PATH = "assets/textures/wall.png";
const char* FLOOR_TEXTURE_PATH = "assets/textures/floor.jpg";
const char* SKY_TEXTURE_PATH = "assets/textures/sky.jpg";

int maze[HEIGHT][WIDTH] = {
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
{1,1,0,0,0,0,0,1,1,0,0,1,0,0,0,1,0,0,0,1,1,1},
{1,1,0,0,0,0,0,1,1,0,1,1,0,1,0,1,0,1,0,0,0,1},
{1,1,0,0,0,0,0,1,1,0,0,0,0,1,0,0,0,1,1,1,0,1},
{1,1,0,0,0,0,0,0,0,0,1,1,0,1,1,1,0,0,0,1,0,1},
{1,1,0,0,0,0,0,1,1,0,1,0,0,0,0,1,1,1,0,1,0,1},
{1,1,0,0,0,0,0,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1},
{1,1,1,1,1,1,1,1,1,0,1,0,0,0,1,0,0,0,0,0,1,1},
{1,1,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,1,1},
{1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0,0,0,1,1},
{1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1},
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int isWall(float x, float y)
{
    int gridX = (int)floorf(x);
    int gridY = (int)floorf(y);

    if (gridX < 0 || gridX >= WIDTH || gridY < 0 || gridY >= HEIGHT)
    {
        return 1;
    }

    return maze[gridY][gridX] == 1;
}

int canMoveTo(float x, float y)
{
    const float radius = 0.18f;

    return !isWall(x - radius, y - radius)
        && !isWall(x + radius, y - radius)
        && !isWall(x - radius, y + radius)
        && !isWall(x + radius, y + radius);
}

GLuint loadTexture(const char* path)
{
    int isFloorTexture = 0;
    GLuint textureId = SOIL_load_OGL_texture(
        path,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
    );

    if (textureId == 0)
    {
        fprintf(stderr, "Failed to load texture: %s\n", path);
        fprintf(stderr, "SOIL error: %s\n", SOIL_last_result());
        return 0;
    }

    if (strcmp(path, FLOOR_TEXTURE_PATH) == 0)
    {
        isFloorTexture = 1;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    {
        GLfloat maxAniso = 0.0f;

        if (isFloorTexture)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        if (maxAniso >= 1.0f)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        }
    }

    return textureId;
}

GLuint loadSkyboxFaceTexture(const char* path)
{
    GLuint textureId = SOIL_load_OGL_texture(
        path,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        0
    );

    if (textureId == 0)
    {
        fprintf(stderr, "Failed to load skybox face: %s\n", path);
        fprintf(stderr, "SOIL error: %s\n", SOIL_last_result());
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureId;
}

void loadSkyTexture(void)
{
    skyTexture = loadSkyboxFaceTexture(SKY_TEXTURE_PATH);
}

void drawFloorAndCeiling(void)
{
    int x;
    int y;

    if (floorTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glColor3f(SCENE_BRIGHTNESS, SCENE_BRIGHTNESS, SCENE_BRIGHTNESS);
    }

    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            if (floorTexture != 0)
            {
                const float floorScale = 1.5f;

                glBegin(GL_QUADS);
                    glTexCoord2f(x * floorScale, y * floorScale); glVertex3f(x * TILE, 0.0f, y * TILE);
                    glTexCoord2f(x * floorScale, (y + 1) * floorScale); glVertex3f(x * TILE, 0.0f, (y + 1) * TILE);
                    glTexCoord2f((x + 1) * floorScale, (y + 1) * floorScale); glVertex3f((x + 1) * TILE, 0.0f, (y + 1) * TILE);
                    glTexCoord2f((x + 1) * floorScale, y * floorScale); glVertex3f((x + 1) * TILE, 0.0f, y * TILE);
                glEnd();
            }
            else
            {
                if ((x + y) % 2 == 0)
                {
                    glColor3f(0.25f, 0.25f, 0.25f);
                }
                else
                {
                    glColor3f(0.18f, 0.18f, 0.18f);
                }

                glBegin(GL_QUADS);
                    glVertex3f(x * TILE, 0.0f, y * TILE);
                    glVertex3f(x * TILE, 0.0f, (y + 1) * TILE);
                    glVertex3f((x + 1) * TILE, 0.0f, (y + 1) * TILE);
                    glVertex3f((x + 1) * TILE, 0.0f, y * TILE);
                glEnd();
            }
        }
    }

    if (floorTexture != 0)
    {
        glDisable(GL_TEXTURE_2D);
    }

}

void drawEndMarker(void)
{
    if (maze[END_TILE_Y][END_TILE_X] == 1)
    {
        return;
    }

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.15f, 0.90f, 0.20f);

    glBegin(GL_QUADS);
        glVertex3f(END_TILE_X * TILE + 0.2f, 0.02f, END_TILE_Y * TILE + 0.2f);
        glVertex3f(END_TILE_X * TILE + 0.2f, 0.02f, END_TILE_Y * TILE + 0.8f);
        glVertex3f(END_TILE_X * TILE + 0.8f, 0.02f, END_TILE_Y * TILE + 0.8f);
        glVertex3f(END_TILE_X * TILE + 0.8f, 0.02f, END_TILE_Y * TILE + 0.2f);
    glEnd();
}

void drawSkybox(float cameraX, float cameraY, float cameraZ)
{
    const float half = SKYBOX_SIZE * 0.5f;
    const float skyY = cameraY + WALL_HEIGHT + 1.0f;

    if (skyTexture == 0)
    {
        return;
    }

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, skyTexture);
    glColor3f(0.28f, 0.28f, 0.32f);

    /* single overhead quad facing down into the maze */
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(cameraX - half, skyY, cameraZ - half);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(cameraX + half, skyY, cameraZ - half);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(cameraX + half, skyY, cameraZ + half);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(cameraX - half, skyY, cameraZ + half);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void drawWall(float x, float y)
{
    int cellX = (int)x;
    int cellY = (int)y;
    int segStart;
    int segEnd;
    float segLen;
    float u0;
    float u1;
    const float midScale = 0.25f;
    float x0 = x * TILE;
    float x1 = x0 + TILE;
    float z0 = y * TILE;
    float z1 = z0 + TILE;
    /* 3-part vertical split: bottom (0.0-0.3), middle repeating (0.3-0.7), top (0.7-1.0) */
    float midH      = WALL_HEIGHT - 2.0f;
    float midRepeat = midH * midScale;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glColor3f(SCENE_BRIGHTNESS, SCENE_BRIGHTNESS, SCENE_BRIGHTNESS);

    /* ---- South face (z1) ---- */
    if (!isWall(cellX + 0.5f, cellY + 1.5f))
    {
        segStart = cellX;
        segEnd = cellX;
        while (segStart > 0
            && maze[cellY][segStart - 1] == 1
            && !isWall((float)segStart - 0.5f, cellY + 1.5f))
        {
            segStart--;
        }
        while (segEnd < WIDTH - 1
            && maze[cellY][segEnd + 1] == 1
            && !isWall((float)segEnd + 1.5f, cellY + 1.5f))
        {
            segEnd++;
        }
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellX - segStart)) / segLen;
        u1 = ((float)(cellX - segStart + 1)) / segLen;

        glBegin(GL_QUADS);
            glTexCoord2f(u0, 0.0f); glVertex3f(x0, 0.0f,              z1);
            glTexCoord2f(u1, 0.0f); glVertex3f(x1, 0.0f,              z1);
            glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f,              z1);
            glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f,              z1);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u0, 0.3f);           glVertex3f(x0, 1.0f,              z1);
            glTexCoord2f(u1, 0.3f);           glVertex3f(x1, 1.0f,              z1);
            glTexCoord2f(u1, 0.3f+midRepeat); glVertex3f(x1, WALL_HEIGHT-1.0f,  z1);
            glTexCoord2f(u0, 0.3f+midRepeat); glVertex3f(x0, WALL_HEIGHT-1.0f,  z1);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u0, 0.7f); glVertex3f(x0, WALL_HEIGHT-1.0f, z1);
            glTexCoord2f(u1, 0.7f); glVertex3f(x1, WALL_HEIGHT-1.0f, z1);
            glTexCoord2f(u1, 1.0f); glVertex3f(x1, WALL_HEIGHT,      z1);
            glTexCoord2f(u0, 1.0f); glVertex3f(x0, WALL_HEIGHT,      z1);
        glEnd();
    }

    /* ---- North face (z0) ---- */
    if (!isWall(cellX + 0.5f, cellY - 0.5f))
    {
        segStart = cellX;
        segEnd = cellX;
        while (segStart > 0
            && maze[cellY][segStart - 1] == 1
            && !isWall((float)segStart - 0.5f, cellY - 0.5f))
        {
            segStart--;
        }
        while (segEnd < WIDTH - 1
            && maze[cellY][segEnd + 1] == 1
            && !isWall((float)segEnd + 1.5f, cellY - 0.5f))
        {
            segEnd++;
        }
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellX - segStart)) / segLen;
        u1 = ((float)(cellX - segStart + 1)) / segLen;

        glBegin(GL_QUADS);
            glTexCoord2f(u1, 0.0f); glVertex3f(x1, 0.0f,              z0);
            glTexCoord2f(u0, 0.0f); glVertex3f(x0, 0.0f,              z0);
            glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f,              z0);
            glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f,              z0);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u1, 0.3f);           glVertex3f(x1, 1.0f,              z0);
            glTexCoord2f(u0, 0.3f);           glVertex3f(x0, 1.0f,              z0);
            glTexCoord2f(u0, 0.3f+midRepeat); glVertex3f(x0, WALL_HEIGHT-1.0f,  z0);
            glTexCoord2f(u1, 0.3f+midRepeat); glVertex3f(x1, WALL_HEIGHT-1.0f,  z0);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u1, 0.7f); glVertex3f(x1, WALL_HEIGHT-1.0f, z0);
            glTexCoord2f(u0, 0.7f); glVertex3f(x0, WALL_HEIGHT-1.0f, z0);
            glTexCoord2f(u0, 1.0f); glVertex3f(x0, WALL_HEIGHT,      z0);
            glTexCoord2f(u1, 1.0f); glVertex3f(x1, WALL_HEIGHT,      z0);
        glEnd();
    }

    /* ---- West face (x0) ---- */
    if (!isWall(cellX - 0.5f, cellY + 0.5f))
    {
        segStart = cellY;
        segEnd = cellY;
        while (segStart > 0
            && maze[segStart - 1][cellX] == 1
            && !isWall(cellX - 0.5f, (float)segStart - 0.5f))
        {
            segStart--;
        }
        while (segEnd < HEIGHT - 1
            && maze[segEnd + 1][cellX] == 1
            && !isWall(cellX - 0.5f, (float)segEnd + 1.5f))
        {
            segEnd++;
        }
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellY - segStart)) / segLen;
        u1 = ((float)(cellY - segStart + 1)) / segLen;

        glBegin(GL_QUADS);
            glTexCoord2f(u0, 0.0f); glVertex3f(x0, 0.0f,              z0);
            glTexCoord2f(u1, 0.0f); glVertex3f(x0, 0.0f,              z1);
            glTexCoord2f(u1, 0.3f); glVertex3f(x0, 1.0f,              z1);
            glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f,              z0);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u0, 0.3f);           glVertex3f(x0, 1.0f,              z0);
            glTexCoord2f(u1, 0.3f);           glVertex3f(x0, 1.0f,              z1);
            glTexCoord2f(u1, 0.3f+midRepeat); glVertex3f(x0, WALL_HEIGHT-1.0f,  z1);
            glTexCoord2f(u0, 0.3f+midRepeat); glVertex3f(x0, WALL_HEIGHT-1.0f,  z0);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u0, 0.7f); glVertex3f(x0, WALL_HEIGHT-1.0f, z0);
            glTexCoord2f(u1, 0.7f); glVertex3f(x0, WALL_HEIGHT-1.0f, z1);
            glTexCoord2f(u1, 1.0f); glVertex3f(x0, WALL_HEIGHT,      z1);
            glTexCoord2f(u0, 1.0f); glVertex3f(x0, WALL_HEIGHT,      z0);
        glEnd();
    }

    /* ---- East face (x1) ---- */
    if (!isWall(cellX + 1.5f, cellY + 0.5f))
    {
        segStart = cellY;
        segEnd = cellY;
        while (segStart > 0
            && maze[segStart - 1][cellX] == 1
            && !isWall(cellX + 1.5f, (float)segStart - 0.5f))
        {
            segStart--;
        }
        while (segEnd < HEIGHT - 1
            && maze[segEnd + 1][cellX] == 1
            && !isWall(cellX + 1.5f, (float)segEnd + 1.5f))
        {
            segEnd++;
        }
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellY - segStart)) / segLen;
        u1 = ((float)(cellY - segStart + 1)) / segLen;

        glBegin(GL_QUADS);
            glTexCoord2f(u1, 0.0f); glVertex3f(x1, 0.0f,              z1);
            glTexCoord2f(u0, 0.0f); glVertex3f(x1, 0.0f,              z0);
            glTexCoord2f(u0, 0.3f); glVertex3f(x1, 1.0f,              z0);
            glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f,              z1);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u1, 0.3f);           glVertex3f(x1, 1.0f,              z1);
            glTexCoord2f(u0, 0.3f);           glVertex3f(x1, 1.0f,              z0);
            glTexCoord2f(u0, 0.3f+midRepeat); glVertex3f(x1, WALL_HEIGHT-1.0f,  z0);
            glTexCoord2f(u1, 0.3f+midRepeat); glVertex3f(x1, WALL_HEIGHT-1.0f,  z1);
        glEnd();
        glBegin(GL_QUADS);
            glTexCoord2f(u1, 0.7f); glVertex3f(x1, WALL_HEIGHT-1.0f, z1);
            glTexCoord2f(u0, 0.7f); glVertex3f(x1, WALL_HEIGHT-1.0f, z0);
            glTexCoord2f(u0, 1.0f); glVertex3f(x1, WALL_HEIGHT,      z0);
            glTexCoord2f(u1, 1.0f); glVertex3f(x1, WALL_HEIGHT,      z1);
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
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

float getLookSurfaceDistance(float maxDistance)
{
    const float step = 0.03f;
    float dirX = cosf(playerAngle) * cosf(playerPitch);
    float dirY = sinf(playerPitch);
    float dirZ = sinf(playerAngle) * cosf(playerPitch);
    float distance = step;

    while (distance <= maxDistance)
    {
        float sampleX = playerX + dirX * distance;
        float sampleHeight = PLAYER_HEIGHT + dirY * distance;
        float sampleZ = playerY + dirZ * distance;

        if (sampleHeight <= 0.0f)
        {
            return distance;
        }

        if (sampleHeight <= WALL_HEIGHT && isWall(sampleX, sampleZ))
        {
            return distance;
        }

        distance += step;
    }

    return maxDistance;
}

void drawFlashlightMask(void)
{
    int i;
    const int segments = 64;
    float centerX;
    float centerY;
    float brightRadius;
    float lookSurfaceDistance;
    float nearDistance;
    float farDistance;
    float farFactor;
    float targetIntensity;
    static float smoothedIntensity = 0.15f;
    static int intensityInitialized = 0;
    const float smoothingFactor = 0.04f;

    if (!flashlightEnabled)
    {
        return;
    }

    centerX = windowWidth * 0.5f;
    centerY = windowHeight * 0.5f;

    lookSurfaceDistance = getLookSurfaceDistance(6.0f);
    nearDistance = 0.2f;
    farDistance = 1.5f;
    farFactor = clampf((lookSurfaceDistance - nearDistance) / (farDistance - nearDistance), 0.0f, 1.0f);
    targetIntensity = 0.28f - farFactor * 0.16f;

    if (!intensityInitialized)
    {
        smoothedIntensity = targetIntensity;
        intensityInitialized = 1;
    }
    else
    {
        smoothedIntensity += (targetIntensity - smoothedIntensity) * smoothingFactor;
    }

    brightRadius = (windowWidth < windowHeight ? windowWidth : windowHeight) * 0.40f;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, (double)windowWidth, 0.0, (double)windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    /* 1) Actual light boost in the center (additive), so where you look is brighter */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.95f, 0.92f, 0.85f, smoothedIntensity);
        glVertex2f(centerX, centerY);
        for (i = 0; i <= segments; i++)
        {
            float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
            float x = centerX + cosf(angle) * brightRadius;
            float y = centerY + sinf(angle) * brightRadius;

            glColor4f(0.95f, 0.92f, 0.85f, 0.0f);
            glVertex2f(x, y);
        }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void reshape(int width, int height)
{
    if (height == 0)
    {
        height = 1;
    }

    windowWidth = width;
    windowHeight = height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(80.0, (double)width / (double)height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    ignoreMouseWarp = 1;
    glutWarpPointer(windowWidth / 2, windowHeight / 2);
}

void display(void)
{
    int x;
    int y;
    float cameraX;
    float cameraY;
    float cameraZ;
    float dirY;
    float flatDir;
    float dirX;
    float dirZ;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    cameraX = playerX;
    cameraY = PLAYER_HEIGHT;
    cameraZ = playerY;
    flatDir = cosf(playerPitch);
    dirX = cosf(playerAngle) * flatDir;
    dirY = sinf(playerPitch);
    dirZ = sinf(playerAngle) * flatDir;

    gluLookAt(cameraX, cameraY, cameraZ,
              cameraX + dirX, cameraY + dirY, cameraZ + dirZ,
              0.0f, 1.0f, 0.0f);

    drawSkybox(cameraX, cameraY, cameraZ);

    drawFloorAndCeiling();

    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            if (maze[y][x] == 1)
            {
                drawWall((float)x, (float)y);
            }
        }
    }

    drawFlashlightMask();

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == 27)
    {
        exit(0);
        return;
    }

    keyStates[(unsigned char)key] = true;

    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    keyStates[(unsigned char)key] = false;
}

void mouseMotion(int x, int y)
{
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    int deltaX;
    int deltaY;

    if (!mouseInitialized)
    {
        mouseInitialized = 1;
        ignoreMouseWarp = 1;
        glutWarpPointer(centerX, centerY);
        return;
    }

    if (ignoreMouseWarp)
    {
        ignoreMouseWarp = 0;
        return;
    }

    deltaX = x - centerX;
    deltaY = y - centerY;
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

        glutPostRedisplay();
    }

    ignoreMouseWarp = 1;
    glutWarpPointer(centerX, centerY);
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
    int playerCellX;
    int playerCellY;

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

    playerCellX = (int)floorf(playerX);
    playerCellY = (int)floorf(playerY);
    if (!endReached && playerCellX == END_TILE_X && playerCellY == END_TILE_Y)
    {
        endReached = true;
        printf("You reached the end of the maze!\n");
    }

    glutPostRedisplay();
}

void init(void)
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.10f, 1.0f);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glutSetCursor(GLUT_CURSOR_NONE);

    wallTexture = loadTexture(WALL_TEXTURE_PATH);
    floorTexture = loadTexture(FLOOR_TEXTURE_PATH);
    loadSkyTexture();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Maze");
    glutFullScreen();

    glutIgnoreKeyRepeat(1);

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(update);

    glutMainLoop();

    return 0;
}