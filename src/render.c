#include <math.h>
#include <stdio.h>
#include <string.h>

#include <SOIL/SOIL.h>

#include "render.h"
#include "gate.h"
#include "glshader.h"

static GLuint loadTextureWithFallback(const char *path, int flags)
{
    static const char *prefixes[] = {"", "../", "../../", "../../../"};
    char candidate[768];
    int i;

    for (i = 0; i < (int)(sizeof(prefixes) / sizeof(prefixes[0])); i++)
    {
        GLuint textureId;

        candidate[0] = '\0';
        snprintf(candidate, sizeof(candidate), "%s%s", prefixes[i], path);
        textureId = SOIL_load_OGL_texture(candidate, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, flags);
        if (textureId != 0)
        {
            if (i != 0)
            {
                fprintf(stderr, "Info: texture loaded via fallback path: %s\n", candidate);
            }
            return textureId;
        }
    }

    fprintf(stderr, "Error: failed to load texture '%s' (SOIL: %s)\n", path, SOIL_last_result());

    return 0;
}

static GLuint loadSkyboxFaceTexture(const char *path)
{
    GLuint textureId = loadTextureWithFallback(path, 0);

    if (textureId == 0)
    {
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureId;
}

GLuint loadTexture(const char *path, int repeatTexture)
{
    GLuint textureId = loadTextureWithFallback(path, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);

    if (textureId == 0)
    {
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    {
        GLfloat maxAniso = 0.0f;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if (repeatTexture)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        if (maxAniso >= 1.0f)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        }
    }

    return textureId;
}

void loadSkyTexture(const char *path)
{
    skyTexture = loadSkyboxFaceTexture(path);
}

void drawFlashlightMask(void)
{
    int i;
    const int segments = 64;
    const float minBeamScale = 0.68f;
    const float maxBeamScale = 1.05f;
    const float minBeamRadiusFactor = 0.22f;
    const float minOverlayIntensity = 0.62f;
    const float innerGlowAlpha = 0.18f;
    const float coreGlowAlpha = 0.82f;
    const float edgeGlowAlpha = 0.36f;
    float centerX = windowWidth * 0.5f;
    float centerY = windowHeight * 0.53f;
    float dirY = sinf(playerPitch);
    float cameraHeight = PLAYER_HEIGHT + cameraBobOffset;
    float downFactor;
    float safeDenom;
    float hitDistance;
    float dynamicScale;
    float distanceT;
    float minDim = (windowWidth < windowHeight ? (float)windowWidth : (float)windowHeight);
    float radius;
    float outerRadius;
    static float flickerTime = 0.0f;
    float intensity;
    float flicker = 1.0f;

    downFactor = clampf(-dirY, 0.0f, 1.0f);
    safeDenom = 0.10f + downFactor * 0.90f;
    hitDistance = cameraHeight / safeDenom;
    distanceT = clampf((hitDistance - 0.55f) / (5.5f - 0.55f), 0.0f, 1.0f);
    dynamicScale = minBeamScale + distanceT * (maxBeamScale - minBeamScale);
    {
        static float smoothedScale = 0.0f;

        if (smoothedScale <= 0.0f)
        {
            smoothedScale = dynamicScale;
        }
        smoothedScale += (dynamicScale - smoothedScale) * 0.08f;
        dynamicScale = smoothedScale;
    }
    radius = minDim * 0.29f * dynamicScale;
    radius = fmaxf(radius, minDim * minBeamRadiusFactor);
    outerRadius = sqrtf((float)(windowWidth * windowWidth + windowHeight * windowHeight)) * 0.72f;
    intensity = fmaxf(0.55f, minOverlayIntensity);

    flickerTime += 0.02f;

    if (!flashlightEnabled)
    {
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
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f((float)windowWidth, 0);
        glVertex2f((float)windowWidth, (float)windowHeight);
        glVertex2f(0, (float)windowHeight);
        glEnd();

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        return;
    }

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

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLE_STRIP);
    for (i = 0; i <= segments; i++)
    {
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
        float cosA = cosf(angle);
        float sinA = sinf(angle);

        glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
        glVertex2f(centerX + cosA * radius, centerY + sinA * radius);

        glColor4f(0.0f, 0.0f, 0.0f, 0.96f);
        glVertex2f(centerX + cosA * outerRadius, centerY + sinA * outerRadius);
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_TRIANGLE_STRIP);
    for (i = 0; i <= segments; i++)
    {
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
        float cosA = cosf(angle);
        float sinA = sinf(angle);

        glColor4f(0.65f, 0.82f, 1.0f, intensity * innerGlowAlpha * flicker);
        glVertex2f(centerX + cosA * (radius * 0.75f), centerY + sinA * (radius * 0.75f));

        glColor4f(0.40f, 0.68f, 1.0f, 0.0f);
        glVertex2f(centerX + cosA * (radius * 1.45f), centerY + sinA * (radius * 1.45f));
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.92f, 0.97f, 1.0f, intensity * coreGlowAlpha * flicker);
    glVertex2f(centerX, centerY);

    for (i = 0; i <= segments; i++)
    {
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
        float x = centerX + cosf(angle) * radius;
        float y = centerY + sinf(angle) * radius;
        float edgeFactor = edgeGlowAlpha;

        glColor4f(0.92f, 0.97f, 1.0f, intensity * edgeFactor * flicker);
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

void drawFloorAndCeiling(void)
{
    int x;
    int y;
    int sx;
    int sy;
    const int floorSubdiv = 12;
    const float invFloorSubdiv = 1.0f / (float)floorSubdiv;
    GLfloat floorDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glMaterialfv(GL_FRONT, GL_DIFFUSE, floorDiffuse);

    if (floorTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        setWorldShaderUseTexture(1);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            if (floorTexture != 0)
            {
                const float floorScale = 1.5f;

                for (sy = 0; sy < floorSubdiv; sy++)
                {
                    for (sx = 0; sx < floorSubdiv; sx++)
                    {
                        float x0 = (float)x + (float)sx * invFloorSubdiv;
                        float x1 = (float)x + (float)(sx + 1) * invFloorSubdiv;
                        float y0 = (float)y + (float)sy * invFloorSubdiv;
                        float y1 = (float)y + (float)(sy + 1) * invFloorSubdiv;

                        glBegin(GL_QUADS);
                        glNormal3f(0.0f, 1.0f, 0.0f);
                        glTexCoord2f(x0 * floorScale, y0 * floorScale); glVertex3f(x0 * TILE, 0.0f, y0 * TILE);
                        glTexCoord2f(x0 * floorScale, y1 * floorScale); glVertex3f(x0 * TILE, 0.0f, y1 * TILE);
                        glTexCoord2f(x1 * floorScale, y1 * floorScale); glVertex3f(x1 * TILE, 0.0f, y1 * TILE);
                        glTexCoord2f(x1 * floorScale, y0 * floorScale); glVertex3f(x1 * TILE, 0.0f, y0 * TILE);
                        glEnd();
                    }
                }
            }
            else
            {
                setWorldShaderUseTexture(0);
                if ((x + y) % 2 == 0)
                {
                    glColor3f(0.5f, 0.5f, 0.5f);
                }
                else
                {
                    glColor3f(0.3f, 0.3f, 0.3f);
                }

                glBegin(GL_QUADS);
                glNormal3f(0.0f, 1.0f, 0.0f);
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

    setWorldShaderUseTexture(0);
    glColor3f(0.08f, 0.08f, 0.10f);
    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            glBegin(GL_QUADS);
            glNormal3f(0.0f, -1.0f, 0.0f);
            glVertex3f(x * TILE, WALL_HEIGHT, y * TILE);
            glVertex3f((x + 1) * TILE, WALL_HEIGHT, y * TILE);
            glVertex3f((x + 1) * TILE, WALL_HEIGHT, (y + 1) * TILE);
            glVertex3f(x * TILE, WALL_HEIGHT, (y + 1) * TILE);
            glEnd();
        }
    }
}

void drawEndMarker(void)
{
    if (maze[endTileY][endTileX] == 1)
    {
        return;
    }

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.15f, 0.90f, 0.20f);

    glBegin(GL_QUADS);
    glVertex3f(endTileX * TILE + 0.2f, 0.02f, endTileY * TILE + 0.2f);
    glVertex3f(endTileX * TILE + 0.2f, 0.02f, endTileY * TILE + 0.8f);
    glVertex3f(endTileX * TILE + 0.8f, 0.02f, endTileY * TILE + 0.8f);
    glVertex3f(endTileX * TILE + 0.8f, 0.02f, endTileY * TILE + 0.2f);
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
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, skyTexture);
    glColor3f(0.28f, 0.28f, 0.32f);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(cameraX - half, skyY, cameraZ - half);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(cameraX + half, skyY, cameraZ - half);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(cameraX + half, skyY, cameraZ + half);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(cameraX - half, skyY, cameraZ + half);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
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
    const float uvEpsilon = 0.008f;
    const float midScale = 0.25f;
    float x0 = x * TILE;
    float x1 = x0 + TILE;
    float z0 = y * TILE;
    float z1 = z0 + TILE;
    GLfloat matDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float midH = WALL_HEIGHT - 2.0f;
    float midRepeat = midH * midScale;

    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);

    if (wallTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, wallTexture);
        setWorldShaderUseTexture(1);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        setWorldShaderUseTexture(0);
        glColor3f(0.62f, 0.62f, 0.66f);
    }

    if (!isWall(cellX + 0.5f, cellY + 1.5f))
    {
        segStart = cellX;
        segEnd = cellX;
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellX - segStart)) / segLen;
        u1 = ((float)(cellX - segStart + 1)) / segLen;
        u0 = u0 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;
        u1 = u1 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;

        glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(u0, 0.0f); glVertex3f(x0, 0.0f, z1);
        glTexCoord2f(u1, 0.0f); glVertex3f(x1, 0.0f, z1);
        glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f, z1);
        glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f, z1);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f, z1);
        glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f, z1);
        glTexCoord2f(u1, 0.3f + midRepeat); glVertex3f(x1, WALL_HEIGHT - 1.0f, z1);
        glTexCoord2f(u0, 0.3f + midRepeat); glVertex3f(x0, WALL_HEIGHT - 1.0f, z1);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(u0, 0.7f); glVertex3f(x0, WALL_HEIGHT - 1.0f, z1);
        glTexCoord2f(u1, 0.7f); glVertex3f(x1, WALL_HEIGHT - 1.0f, z1);
        glTexCoord2f(u1, 1.0f); glVertex3f(x1, WALL_HEIGHT, z1);
        glTexCoord2f(u0, 1.0f); glVertex3f(x0, WALL_HEIGHT, z1);
        glEnd();
    }

    if (!isWall(cellX + 0.5f, cellY - 0.5f))
    {
        segStart = cellX;
        segEnd = cellX;
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellX - segStart)) / segLen;
        u1 = ((float)(cellX - segStart + 1)) / segLen;
        u0 = u0 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;
        u1 = u1 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;

        glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(u1, 0.0f); glVertex3f(x1, 0.0f, z0);
        glTexCoord2f(u0, 0.0f); glVertex3f(x0, 0.0f, z0);
        glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f, z0);
        glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f, z0);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f, z0);
        glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f, z0);
        glTexCoord2f(u0, 0.3f + midRepeat); glVertex3f(x0, WALL_HEIGHT - 1.0f, z0);
        glTexCoord2f(u1, 0.3f + midRepeat); glVertex3f(x1, WALL_HEIGHT - 1.0f, z0);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(u1, 0.7f); glVertex3f(x1, WALL_HEIGHT - 1.0f, z0);
        glTexCoord2f(u0, 0.7f); glVertex3f(x0, WALL_HEIGHT - 1.0f, z0);
        glTexCoord2f(u0, 1.0f); glVertex3f(x0, WALL_HEIGHT, z0);
        glTexCoord2f(u1, 1.0f); glVertex3f(x1, WALL_HEIGHT, z0);
        glEnd();
    }

    if (!isWall(cellX - 0.5f, cellY + 0.5f))
    {
        segStart = cellY;
        segEnd = cellY;
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellY - segStart)) / segLen;
        u1 = ((float)(cellY - segStart + 1)) / segLen;
        u0 = u0 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;
        u1 = u1 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;

        glBegin(GL_QUADS);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(u0, 0.0f); glVertex3f(x0, 0.0f, z0);
        glTexCoord2f(u1, 0.0f); glVertex3f(x0, 0.0f, z1);
        glTexCoord2f(u1, 0.3f); glVertex3f(x0, 1.0f, z1);
        glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f, z0);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(u0, 0.3f); glVertex3f(x0, 1.0f, z0);
        glTexCoord2f(u1, 0.3f); glVertex3f(x0, 1.0f, z1);
        glTexCoord2f(u1, 0.3f + midRepeat); glVertex3f(x0, WALL_HEIGHT - 1.0f, z1);
        glTexCoord2f(u0, 0.3f + midRepeat); glVertex3f(x0, WALL_HEIGHT - 1.0f, z0);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(u0, 0.7f); glVertex3f(x0, WALL_HEIGHT - 1.0f, z0);
        glTexCoord2f(u1, 0.7f); glVertex3f(x0, WALL_HEIGHT - 1.0f, z1);
        glTexCoord2f(u1, 1.0f); glVertex3f(x0, WALL_HEIGHT, z1);
        glTexCoord2f(u0, 1.0f); glVertex3f(x0, WALL_HEIGHT, z0);
        glEnd();
    }

    if (!isWall(cellX + 1.5f, cellY + 0.5f))
    {
        segStart = cellY;
        segEnd = cellY;
        segLen = (float)(segEnd - segStart + 1);
        u0 = ((float)(cellY - segStart)) / segLen;
        u1 = ((float)(cellY - segStart + 1)) / segLen;
        u0 = u0 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;
        u1 = u1 * (1.0f - 2.0f * uvEpsilon) + uvEpsilon;

        glBegin(GL_QUADS);
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(u1, 0.0f); glVertex3f(x1, 0.0f, z1);
        glTexCoord2f(u0, 0.0f); glVertex3f(x1, 0.0f, z0);
        glTexCoord2f(u0, 0.3f); glVertex3f(x1, 1.0f, z0);
        glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f, z1);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(u1, 0.3f); glVertex3f(x1, 1.0f, z1);
        glTexCoord2f(u0, 0.3f); glVertex3f(x1, 1.0f, z0);
        glTexCoord2f(u0, 0.3f + midRepeat); glVertex3f(x1, WALL_HEIGHT - 1.0f, z0);
        glTexCoord2f(u1, 0.3f + midRepeat); glVertex3f(x1, WALL_HEIGHT - 1.0f, z1);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(u1, 0.7f); glVertex3f(x1, WALL_HEIGHT - 1.0f, z1);
        glTexCoord2f(u0, 0.7f); glVertex3f(x1, WALL_HEIGHT - 1.0f, z0);
        glTexCoord2f(u0, 1.0f); glVertex3f(x1, WALL_HEIGHT, z0);
        glTexCoord2f(u1, 1.0f); glVertex3f(x1, WALL_HEIGHT, z1);
        glEnd();
    }

    if (wallTexture != 0)
    {
        glDisable(GL_TEXTURE_2D);
    }
    setWorldShaderUseTexture(0);
}

void drawGatePrompt(void)
{
    const char *msg = "Press [E] to open the door.";
    int msgLen;
    int charWidth;
    int textWidth;
    float sx;
    float sy;
    int i;

    if (!canInteractWithStarterGate())
    {
        return;
    }

    msgLen = (int)strlen(msg);
    charWidth = 9;
    textWidth = msgLen * charWidth;
    sx = (float)(windowWidth - textWidth) * 0.5f;
    sy = (float)windowHeight * 0.5f;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)windowWidth, 0.0, (double)windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.0f, 0.85f);
    glRasterPos2f(sx + 2.0f, sy - 2.0f);
    for (i = 0; i < msgLen; i++)
    {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, msg[i]);
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glRasterPos2f(sx, sy);
    for (i = 0; i < msgLen; i++)
    {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, msg[i]);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}