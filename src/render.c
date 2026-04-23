#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <SDL_image.h>
#include <GL/glu.h>

#include "render.h"

static GLuint gFontBase = 0;
static int gFontReady = 0;

static void drawOverlayText(float x, float y, const char* text)
{
    if (!gFontReady || !text)
    {
        return;
    }

    glRasterPos2f(x, y);
    glListBase(gFontBase - 32);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
}

int initBitmapFont(void)
{
    HDC dc;

    if (gFontReady)
    {
        return 1;
    }

    dc = wglGetCurrentDC();
    if (!dc)
    {
        return 0;
    }

    gFontBase = glGenLists(96);
    if (gFontBase == 0)
    {
        return 0;
    }

    if (!wglUseFontBitmapsA(dc, 32, 96, gFontBase))
    {
        glDeleteLists(gFontBase, 96);
        gFontBase = 0;
        return 0;
    }

    gFontReady = 1;
    return 1;
}

void shutdownBitmapFont(void)
{
    if (gFontReady)
    {
        glDeleteLists(gFontBase, 96);
        gFontBase = 0;
        gFontReady = 0;
    }
}

GLuint loadTexture(const char* path, int isFloorTexture)
{
    GLuint textureId = 0;
    SDL_Surface* surface = IMG_Load(path);

    if (!surface)
    {
        return 0;
    }

    GLenum format = 0;
    if (surface->format->BytesPerPixel == 4)
    {
        format = (surface->format->Rmask == 0x000000ff) ? GL_RGBA : GL_BGRA;
    }
    else if (surface->format->BytesPerPixel == 3)
    {
        format = (surface->format->Rmask == 0x000000ff) ? GL_RGB : GL_BGR;
    }
    else
    {
        SDL_FreeSurface(surface);
        return 0;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    if (gluBuild2DMipmaps(GL_TEXTURE_2D,
        format,
        surface->w,
        surface->h,
        format,
        GL_UNSIGNED_BYTE,
        surface->pixels) != 0)
    {
        SDL_FreeSurface(surface);
        return 0;
    }

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

    {
        GLfloat maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        if (maxAniso >= 1.0f)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        }
    }

    SDL_FreeSurface(surface);
    return textureId;
}

static void drawHelpOverlay(void)
{
    if (!showHelp)
    {
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.0f, 0.72f);
    glBegin(GL_QUADS);
    glVertex2f(90.0f, 90.0f);
    glVertex2f((float)windowWidth - 90.0f, 90.0f);
    glVertex2f((float)windowWidth - 90.0f, (float)windowHeight - 90.0f);
    glVertex2f(90.0f, (float)windowHeight - 90.0f);
    glEnd();

    glDisable(GL_BLEND);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawOverlayText(130.0f, 145.0f, "HASZNALATI UTMUTATO (F1)");
    drawOverlayText(130.0f, 200.0f, "Mozgas: W A S D");
    drawOverlayText(130.0f, 240.0f, "Fenyero noveles: FEL NYIL");
    drawOverlayText(130.0f, 280.0f, "Fenyero csokkentes: LE NYIL");
    drawOverlayText(130.0f, 320.0f, "Kilepes: ESC");

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static void drawFloorAndCeiling(void)
{
    if (!floorTexture)
    {
        for (int y = 0; y < HEIGHT; y++)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                float c = ((x + y) & 1) ? 0.25f : 0.18f;
                glColor3f(c, c, c);
                glBegin(GL_QUADS);
                glVertex3f(x, 0, y);
                glVertex3f(x, 0, y + 1);
                glVertex3f(x + 1, 0, y + 1);
                glVertex3f(x + 1, 0, y);
                glEnd();
            }
        }
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glColor3f(flashlightIntensity, flashlightIntensity, flashlightIntensity);

    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            float s = x * 1.5f;
            float t = y * 1.5f;
            glBegin(GL_QUADS);
            glTexCoord2f(s, t); glVertex3f(x, 0, y);
            glTexCoord2f(s, t + 1.5f); glVertex3f(x, 0, y + 1);
            glTexCoord2f(s + 1.5f, t + 1.5f); glVertex3f(x + 1, 0, y + 1);
            glTexCoord2f(s + 1.5f, t); glVertex3f(x + 1, 0, y);
            glEnd();
        }
    }
    glDisable(GL_TEXTURE_2D);
}

static void drawSkybox(float cameraX, float cameraY, float cameraZ)
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

static void drawWall(float x, float y)
{
    int cellX = (int)x;
    int cellY = (int)y;

    float x0 = x * TILE;
    float x1 = (x + 1.0f) * TILE;
    float z0 = y * TILE;
    float z1 = (y + 1.0f) * TILE;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glColor3f(flashlightIntensity, flashlightIntensity, flashlightIntensity);

    if (!isWall(cellX + 0.5f, cellY + 1.5f))
    {
        glNormal3f(0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x0, 0.0f, z1);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(x1, 0.0f, z1);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(x1, WALL_HEIGHT, z1);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(x0, WALL_HEIGHT, z1);
        glEnd();
    }

    if (!isWall(cellX + 0.5f, cellY - 0.5f))
    {
        glNormal3f(0.0f, 0.0f, -1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, 0.0f, z0);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(x0, 0.0f, z0);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(x0, WALL_HEIGHT, z0);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, WALL_HEIGHT, z0);
        glEnd();
    }

    if (!isWall(cellX - 0.5f, cellY + 0.5f))
    {
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x0, 0.0f, z0);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(x0, 0.0f, z1);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(x0, WALL_HEIGHT, z1);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(x0, WALL_HEIGHT, z0);
        glEnd();
    }

    if (!isWall(cellX + 1.5f, cellY + 0.5f))
    {
        glNormal3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, 0.0f, z1);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(x1, 0.0f, z0);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(x1, WALL_HEIGHT, z0);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, WALL_HEIGHT, z1);
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
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

    glEnable(GL_NORMALIZE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    cameraX = playerX;
    cameraY = PLAYER_HEIGHT + cameraBobOffset;
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

    if (gTableModel.displayList != 0)
    {
        glPushMatrix();
        glTranslatef(tableX, 0.0f, tableZ);
        glScalef(0.01f, 0.01f, 0.01f);

        if (tableTexture != 0)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tableTexture);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }

        glColor3f(flashlightIntensity, flashlightIntensity, flashlightIntensity);
        glCallList(gTableModel.displayList);

        if (tableTexture != 0)
        {
            glDisable(GL_TEXTURE_2D);
        }

        glPopMatrix();
    }

    drawHelpOverlay();
}