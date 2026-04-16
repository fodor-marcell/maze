#include <windows.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <GL/glu.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

static SDL_Window* gWindow = NULL;
static SDL_GLContext gGLContext = NULL;
static int gRunning = 1;

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

float playerX = 4.5f;
float playerY = 5.5f;
float playerAngle = 0.0f;
float playerPitch = 0.0f;
float cameraBobOffset = 0.0f;
float walkBobPhase = 0.0f;
float tableAngle = 0.0f;
float tableX = 3.2f;
float tableZ = 5.5f;
const float tableBlockHalfX = 0.95f;
const float tableBlockHalfZ = 0.65f;
int windowWidth = 1000;
int windowHeight = 700;
int mouseInitialized = 0;
int ignoreMouseWarp = 0;
bool keyStates[256] = { false };
bool showHelp = false;

GLuint wallTexture = 0;
GLuint floorTexture = 0;
GLuint skyTexture = 0;
GLuint tableTexture = 0;
Model gTableModel = {0};
float flashlightIntensity = SCENE_BRIGHTNESS;
static GLuint gFontBase = 0;
static int gFontReady = 0;

static int initBitmapFont(void)
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

int loadOBJ(const char* path, Model* model)
{
    FILE* f;
    char line[512];
    int vCount = 0;
    int vtCount = 0;
    int vnCount = 0;
    int fCount = 0;
    int vIdx = 0;
    int vtIdx = 0;
    int vnIdx = 0;
    int fIdx = 0;

    if (!path || !model)
    {
        return 0;
    }

    memset(model, 0, sizeof(*model));

    f = fopen(path, "r");
    if (!f)
    {
        return 0;
    }

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "v ", 2) == 0)
        {
            vCount++;
        }
        else if (strncmp(line, "vt ", 3) == 0)
        {
            vtCount++;
        }
        else if (strncmp(line, "vn ", 3) == 0)
        {
            vnCount++;
        }
        else if (strncmp(line, "f ", 2) == 0)
        {
            fCount++;
        }
    }
    rewind(f);

    model->vertices = (Vec3*)malloc((size_t)vCount * sizeof(Vec3));
    model->texcoords = (Vec2*)malloc((size_t)vtCount * sizeof(Vec2));
    model->normals = (Vec3*)malloc((size_t)vnCount * sizeof(Vec3));
    model->faces = (Face*)malloc((size_t)fCount * sizeof(Face));

    if ((vCount > 0 && !model->vertices) ||
        (vtCount > 0 && !model->texcoords) ||
        (vnCount > 0 && !model->normals) ||
        (fCount > 0 && !model->faces))
    {
        fclose(f);
        free(model->vertices);
        free(model->texcoords);
        free(model->normals);
        free(model->faces);
        memset(model, 0, sizeof(*model));
        return 0;
    }

    model->vertexCount = vCount;
    model->texcoordCount = vtCount;
    model->normalCount = vnCount;
    model->faceCount = fCount;

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "v ", 2) == 0)
        {
            sscanf(line, "v %f %f %f",
                   &model->vertices[vIdx].x,
                   &model->vertices[vIdx].y,
                   &model->vertices[vIdx].z);
            vIdx++;
        }
        else if (strncmp(line, "vt ", 3) == 0)
        {
            sscanf(line, "vt %f %f",
                   &model->texcoords[vtIdx].u,
                   &model->texcoords[vtIdx].v);
            vtIdx++;
        }
        else if (strncmp(line, "vn ", 3) == 0)
        {
            sscanf(line, "vn %f %f %f",
                   &model->normals[vnIdx].x,
                   &model->normals[vnIdx].y,
                   &model->normals[vnIdx].z);
            vnIdx++;
        }
        else if (strncmp(line, "f ", 2) == 0)
        {
            if (sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                       &model->faces[fIdx].v[0], &model->faces[fIdx].vt[0], &model->faces[fIdx].vn[0],
                       &model->faces[fIdx].v[1], &model->faces[fIdx].vt[1], &model->faces[fIdx].vn[1],
                       &model->faces[fIdx].v[2], &model->faces[fIdx].vt[2], &model->faces[fIdx].vn[2]) == 9)
            {
                fIdx++;
            }
        }
    }
    fclose(f);

    model->faceCount = fIdx;
    model->displayList = glGenLists(1);
    if (model->displayList == 0)
    {
        return 0;
    }

    glNewList(model->displayList, GL_COMPILE);
    for (int i = 0; i < model->faceCount; i++)
    {
        glBegin(GL_TRIANGLES);
        for (int j = 0; j < 3; j++)
        {
            int vi = model->faces[i].v[j] - 1;
            int vti = model->faces[i].vt[j] - 1;
            int vni = model->faces[i].vn[j] - 1;

            if (vti >= 0 && vti < model->texcoordCount)
            {
                glTexCoord2f(model->texcoords[vti].u, model->texcoords[vti].v);
            }
            if (vni >= 0 && vni < model->normalCount)
            {
                glNormal3f(model->normals[vni].x, model->normals[vni].y, model->normals[vni].z);
            }
            if (vi >= 0 && vi < model->vertexCount)
            {
                glVertex3f(model->vertices[vi].x, model->vertices[vi].y, model->vertices[vi].z);
            }
        }
        glEnd();
    }
    glEndList();

    return 1;
}

void freeModel(Model* model)
{
    if (!model)
    {
        return;
    }

    free(model->vertices);
    free(model->texcoords);
    free(model->normals);
    free(model->faces);

    if (model->displayList)
    {
        glDeleteLists(model->displayList, 1);
    }

    memset(model, 0, sizeof(*model));
}

int isWall(float x, float y)
{
    int gridX = (int)floorf(x);
    int gridY = (int)floorf(y);
    
    if (x < 0.01f || x >= WIDTH - 0.01f || y < 0.01f || y >= HEIGHT - 0.01f) {
        return 1;
    }
    
    if (gridX < 0 || gridX >= WIDTH || gridY < 0 || gridY >= HEIGHT) {
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

static GLuint loadTexture(const char* path, int isFloorTexture)
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

static void loadSkyTexture(const char* path)
{
    skyTexture = loadTexture(path, 0);
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

void drawFloorAndCeiling(void) {
    if (!floorTexture) {
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++) {
                float c = ((x+y)&1) ? 0.25f : 0.18f;
                glColor3f(c,c,c);
                glBegin(GL_QUADS);
                    glVertex3f(x,0,y); glVertex3f(x,0,y+1);
                    glVertex3f(x+1,0,y+1); glVertex3f(x+1,0,y);
                glEnd();
            }
        return;
    }
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glColor3f(flashlightIntensity, flashlightIntensity, flashlightIntensity);
    
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            float s = x * 1.5f, t = y * 1.5f;
            glBegin(GL_QUADS);
                glTexCoord2f(s,t); glVertex3f(x,0,y);
                glTexCoord2f(s,t+1.5f); glVertex3f(x,0,y+1);
                glTexCoord2f(s+1.5f,t+1.5f); glVertex3f(x+1,0,y+1);
                glTexCoord2f(s+1.5f,t); glVertex3f(x+1,0,y);
            glEnd();
        }
    glDisable(GL_TEXTURE_2D);
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

    float x0 = x * TILE;
    float x1 = (x + 1.0f) * TILE;
    float z0 = y * TILE;
    float z1 = (y + 1.0f) * TILE;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glColor3f(flashlightIntensity, flashlightIntensity, flashlightIntensity);

    /* South */
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

    /* North */
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

    /* West */
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

    /* East */
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
    int playerCellX;
    int playerCellY;
    static int lastTime = 0;
    static float prevCameraBobOffset = 0.0f;
    static int wasMovingLastFrame = 0;
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
        wasMovingLastFrame = 0;
    }

    prevCameraBobOffset = cameraBobOffset;

    playerCellX = (int)floorf(playerX);
    playerCellY = (int)floorf(playerY);

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
    loadSkyTexture(SKY_TEXTURE_PATH);

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

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return 1;
    }

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags)
    {
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    gWindow = SDL_CreateWindow(
        "Maze - Labyrinthine Style Flashlight",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1000,
        700,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (!gWindow)
    {
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    gGLContext = SDL_GL_CreateContext(gWindow);
    if (!gGLContext)
    {
        SDL_DestroyWindow(gWindow);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);

    SDL_GetWindowSize(gWindow, &windowWidth, &windowHeight);
    reshape(windowWidth, windowHeight);

    init();

    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    gRunning = 1;
    while (gRunning)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    gRunning = 0;
                    break;
                case SDL_KEYDOWN:
					keyboard(e.key.keysym.sym, 0, 0);
                    break;
                case SDL_KEYUP:
					keyboardUp(e.key.keysym.sym, 0, 0);
                    break;
                case SDL_MOUSEMOTION:
					mouseMotion(e.motion.xrel, e.motion.yrel);
                    break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        reshape(e.window.data1, e.window.data2);
                    }
                    break;
                default:
                    break;
            }
        }

        update();
        display();
        SDL_GL_SwapWindow(gWindow);
    }

    if (gFontReady)
    {
        glDeleteLists(gFontBase, 96);
    }

    freeModel(&gTableModel);

    if (tableTexture != 0)
    {
        glDeleteTextures(1, &tableTexture);
    }

    SDL_GL_DeleteContext(gGLContext);
    SDL_DestroyWindow(gWindow);
    IMG_Quit();
    SDL_Quit();

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    return main(0, NULL);
}
#endif