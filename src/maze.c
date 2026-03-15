#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "maze_shared.h"
#include <mmsystem.h>
#include "gate.h"
#include "maze_generator.h"
#include "glshader.h"
#include "render.h"


float playerX = 4.5f;
float playerY = 5.5f;
float playerAngle = 0.0f;
float playerPitch = 0.0f;
float cameraBobOffset = 0.0f;
float walkBobPhase = 0.0f;
int windowWidth = 1000;
int windowHeight = 700;
int mouseInitialized = 0;
int ignoreMouseWarp = 0;
bool keyStates[256] = { false };
bool endReached = false;
bool flashlightEnabled = true;
float monsterX = 1.5f;
float monsterY = 1.5f;
float monsterAngle = 0.0f;
float monsterSpeed = 0.012f;
float monsterTargetX = 6.5f;
float monsterTargetY = 6.5f;
int monsterState = 0;
float monsterThinkTimer = 0.0f;


float flashlightIntensity = 1.0f;

GLuint wallTexture = 0;
GLuint floorTexture = 0;
GLuint skyTexture = 0;
GLuint gateTexture = 0;
GLuint skeletonTexture = 0;
GLuint zombieTexture = 0;

const char* WALL_TEXTURE_PATH = "assets/textures/wall.png";
const char* FLOOR_TEXTURE_PATH = "assets/textures/floor.jpg";
const char* SKY_TEXTURE_PATH = "assets/textures/sky.jpg";
const char* GATE_TEXTURE_PATH = "assets/textures/wall.png";
const char* GATE_OBJ_PATH     = "assets/textures/gate.obj";
const char* SKELETON_OBJ_PATH = "assets/textures/skeleton.obj";
const char* SKELETON_TEXTURE_PATH = "assets/textures/skeleton-sitting/textures/Skeletor_DECILP_Wrapped_standardSurface1_B.png";
const char* ZOMBIE_OBJ_PATH = "assets/textures/Wendigo.obj";
const char* ZOMBIE_TEXTURE_PATH = "assets/textures/models/wendigo.png";
const char* AUDIO_CANDIDATES[] = {
    "assets/audio/universfield-horror-background-atmosphere-156462.mp3",
    "assets/audio/horror_ambient.wav",
    "assets/audio/bgm.wav"
};
const int AUDIO_CANDIDATE_COUNT = 3;
const char* GATE_SOUND_CANDIDATES[] = {
    "assets/audio/gate-open.mp3"
};
const int GATE_SOUND_CANDIDATE_COUNT = 1;
const char* WALK_SOUND_CANDIDATES[] = {
    "assets/audio/walking.mp3",
    "assets/audio/walking.wav"
};
const int WALK_SOUND_CANDIDATE_COUNT = 2;
const char* CROW_SOUND_CANDIDATES[] = {
    "assets/audio/crow.mp3",
    "assets/audio/crow.wav"
};
const int CROW_SOUND_CANDIDATE_COUNT = 2;


const char* MONSTER_SPOT_SOUND_CANDIDATES[] = {
    "assets/audio/wendigo_scream.mp3",
};
const int MONSTER_SPOT_SOUND_CANDIDATE_COUNT = 1;


const char* MONSTER_WALK_SOUND_CANDIDATES[] = {
    "assets/audio/wendigo_walking.mp3",
};
const int MONSTER_WALK_SOUND_CANDIDATE_COUNT = 1;

static int mciAudioOpen = 0;
static const char* activeWalkSoundPath = NULL;
static const char* activeCrowSoundPath = NULL;
static float crowTimerSec = 0.0f;
static const char* activeMonsterSpotSoundPath = NULL;
static float monsterSpotCooldown = 0.0f;
static const char* activeMonsterWalkSoundPath = NULL;
static float monsterWalkTimer = 0.0f;
static int monsterWasMovingLastFrame = 0;
static const char* activeMonsterAmbientPath = NULL;
static float monsterProximityTimer = 0.0f;
static int monsterProximityPlaying = 0;
static float monsterProximityCheckTimer = 0.0f;


void updateMonster(float deltaTime);
int monsterCanSeePlayer();
void monsterPickRandomTarget();
void drawMonster();
void findSafeMonsterSpawn();
static void playMonsterWalkSound(void);

static int playMp3Loop(const char* path)
{
    char command[1024];
    MCIERROR err;
    char errText[256];

    snprintf(command, sizeof(command), "open \"%s\" type mpegvideo alias bgmAudio", path);
    err = mciSendStringA(command, NULL, 0, NULL);
    if (err != 0)
    {
        mciGetErrorStringA(err, errText, (UINT)sizeof(errText));
        fprintf(stderr, "Warning: cannot open MP3 (%s): %s\n", path, errText);
        return 0;
    }

    err = mciSendStringA("play bgmAudio repeat", NULL, 0, NULL);
    if (err != 0)
    {
        mciGetErrorStringA(err, errText, (UINT)sizeof(errText));
        fprintf(stderr, "Warning: cannot play MP3 (%s): %s\n", path, errText);
        mciSendStringA("close bgmAudio", NULL, 0, NULL);
        return 0;
    }

    mciAudioOpen = 1;
    return 1;
}

int endTileX = 14;
int endTileY = 10;
int startTileX = 1;
int startTileY = 1;

int maze[HEIGHT][WIDTH];

typedef struct SkeletonInstance {
    float x;
    float z;
    float faceX;
    float faceZ;
} SkeletonInstance;

static const float skeletonWidth = 0.21f;
static const float skeletonHeight = 0.36f;
static const float skeletonWallOffset = 0.30f;
static const float skeletonMeshPushOut = 0.18f;
static const float skeletonBlockRadius = 0.16f;
static SkeletonInstance skeletonInstances[WIDTH * HEIGHT];
static int skeletonInstanceCount = 0;

static int isDeadEndTile(int cellX, int cellY, int* openDirX, int* openDirY)
{
    int openCount = 0;
    int foundOpenX = 0;
    int foundOpenY = 0;
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int i;

    if (maze[cellY][cellX] != 0)
    {
        return 0;
    }

    for (i = 0; i < 4; i++)
    {
        int nx = cellX + dirs[i][0];
        int ny = cellY + dirs[i][1];
        if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
        {
            continue;
        }
        if (maze[ny][nx] == 0)
        {
            openCount++;
            foundOpenX = dirs[i][0];
            foundOpenY = dirs[i][1];
        }
    }

    if (openCount == 1)
    {
        if (openDirX != NULL)
        {
            *openDirX = foundOpenX;
        }
        if (openDirY != NULL)
        {
            *openDirY = foundOpenY;
        }
        return 1;
    }

    return 0;
}

static void rebuildSkeletonInstances(void)
{
    int x;
    int y;

    skeletonInstanceCount = 0;

    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            int openDirX;
            int openDirY;
            float centerX;
            float centerZ;
            float normalX;
            float normalZ;
            float px;
            float pz;

            if (x == endTileX && y == endTileY)
            {
                continue;
            }
            if (!isDeadEndTile(x, y, &openDirX, &openDirY))
            {
                continue;
            }
            if (skeletonInstanceCount >= WIDTH * HEIGHT)
            {
                return;
            }

            centerX = ((float)x + 0.5f) * TILE;
            centerZ = ((float)y + 0.5f) * TILE;
            normalX = (float)openDirX;
            normalZ = (float)openDirY;
            px = centerX - normalX * skeletonWallOffset;
            pz = centerZ - normalZ * skeletonWallOffset;

            skeletonInstances[skeletonInstanceCount].x = px + normalX * skeletonMeshPushOut;
            skeletonInstances[skeletonInstanceCount].z = pz + normalZ * skeletonMeshPushOut;
            skeletonInstances[skeletonInstanceCount].faceX = normalX;
            skeletonInstances[skeletonInstanceCount].faceZ = normalZ;
            skeletonInstanceCount++;
        }
    }
}

static void drawSkeletonQuad(float px, float pz, float normalX, float normalZ, float tangentX, float tangentZ, float halfWidth, float height)
{
    glBegin(GL_QUADS);
    glNormal3f(normalX, 0.0f, normalZ);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(px - tangentX * halfWidth, 0.0f, pz - tangentZ * halfWidth);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(px + tangentX * halfWidth, 0.0f, pz + tangentZ * halfWidth);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(px + tangentX * halfWidth, height, pz + tangentZ * halfWidth);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(px - tangentX * halfWidth, height, pz - tangentZ * halfWidth);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(-normalX, 0.0f, -normalZ);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(px - tangentX * halfWidth, 0.0f, pz - tangentZ * halfWidth);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(px + tangentX * halfWidth, 0.0f, pz + tangentZ * halfWidth);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(px + tangentX * halfWidth, height, pz + tangentZ * halfWidth);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(px - tangentX * halfWidth, height, pz - tangentZ * halfWidth);
    glEnd();
}

static void drawSkeletonCrossProp(float px, float pz, float normalX, float normalZ, float tangentX, float tangentZ, float halfWidth, float height)
{
    const float anglesDeg[3] = {0.0f, 55.0f, -55.0f};
    int i;

    for (i = 0; i < 3; i++)
    {
        float a = anglesDeg[i] * (float)M_PI / 180.0f;
        float c = cosf(a);
        float s = sinf(a);
        float rnX = normalX * c + tangentX * s;
        float rnZ = normalZ * c + tangentZ * s;
        float rtX = -normalX * s + tangentX * c;
        float rtZ = -normalZ * s + tangentZ * c;

        drawSkeletonQuad(px, pz, rnX, rnZ, rtX, rtZ, halfWidth, height);
    }
}

static void drawDeadEndSkeletonProps(void)
{
    const float maxRenderDistance = 6.0f;
    const int maxMeshDrawCount = 3;
    int useMesh = hasSkeletonMesh();
    int oldCullEnabled = glIsEnabled(GL_CULL_FACE);
    int oldBlendEnabled = glIsEnabled(GL_BLEND);
    int oldAlphaEnabled = glIsEnabled(GL_ALPHA_TEST);
    int meshDrawCount = 0;
    int i;

    if (!useMesh && skeletonTexture == 0)
    {
        return;
    }

    if (!useMesh)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, skeletonTexture);
        setWorldShaderUseTexture(1);
        glColor3f(1.0f, 1.0f, 1.0f);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.20f);
    }

    for (i = 0; i < skeletonInstanceCount; i++)
    {
        float drawX = skeletonInstances[i].x;
        float drawZ = skeletonInstances[i].z;
        float normalX = skeletonInstances[i].faceX;
        float normalZ = skeletonInstances[i].faceZ;
        float tangentX = -normalZ;
        float tangentZ = normalX;
        float halfWidth = skeletonWidth * 0.5f;
        float px = drawX - normalX * skeletonMeshPushOut;
        float pz = drawZ - normalZ * skeletonMeshPushOut;
        float dx = drawX - playerX;
        float dz = drawZ - playerY;

        if (useMesh)
        {
            if ((dx * dx + dz * dz) > (maxRenderDistance * maxRenderDistance))
            {
                continue;
            }
            if (meshDrawCount >= maxMeshDrawCount)
            {
                continue;
            }

            drawSkeletonMeshAt(drawX, drawZ, normalX, normalZ, skeletonHeight);
            meshDrawCount++;
        }
        else
        {
            drawSkeletonCrossProp(px, pz, normalX, normalZ, tangentX, tangentZ, halfWidth, skeletonHeight);
        }
    }

    if (!useMesh && !oldAlphaEnabled)
    {
        glDisable(GL_ALPHA_TEST);
    }
    if (!useMesh && !oldBlendEnabled)
    {
        glDisable(GL_BLEND);
    }
    if (!useMesh && oldCullEnabled)
    {
        glEnable(GL_CULL_FACE);
    }

    if (!useMesh)
    {
        setWorldShaderUseTexture(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
}

static int isSkeletonBlockingPosition(float x, float y, float radius)
{
    int i;

    if (!hasSkeletonMesh() && skeletonTexture == 0)
    {
        return 0;
    }

    for (i = 0; i < skeletonInstanceCount; i++)
    {
        float dx = x - skeletonInstances[i].x;
        float dz = y - skeletonInstances[i].z;
        float blockR = radius + skeletonBlockRadius;

        if ((dx * dx + dz * dz) <= (blockR * blockR))
        {
            return 1;
        }
    }

    return 0;
}

static void stopBackgroundMusic(void)
{
    if (mciAudioOpen)
    {
        mciSendStringA("stop bgmAudio", NULL, 0, NULL);
        mciSendStringA("close bgmAudio", NULL, 0, NULL);
        mciAudioOpen = 0;
    }

    mciSendStringA("stop gateSfx", NULL, 0, NULL);
    mciSendStringA("close gateSfx", NULL, 0, NULL);
    mciSendStringA("stop stepSfx", NULL, 0, NULL);
    mciSendStringA("close stepSfx", NULL, 0, NULL);
    mciSendStringA("stop crowSfx", NULL, 0, NULL);
    mciSendStringA("close crowSfx", NULL, 0, NULL);
    mciSendStringA("stop monsterSfx", NULL, 0, NULL);
    mciSendStringA("close monsterSfx", NULL, 0, NULL);
    mciSendStringA("stop monsterAmbientSfx", NULL, 0, NULL);
    mciSendStringA("close monsterAmbientSfx", NULL, 0, NULL);
    mciSendStringA("stop monsterStepSfx", NULL, 0, NULL);
    mciSendStringA("close monsterStepSfx", NULL, 0, NULL);

    PlaySoundA(NULL, NULL, 0);
}

static float randomRangef(float minVal, float maxVal)
{
    float t = (float)rand() / (float)RAND_MAX;
    return minVal + (maxVal - minVal) * t;
}

static const char* findFirstExistingAudio(const char* const* candidates, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        DWORD attrs = GetFileAttributesA(candidates[i]);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            return candidates[i];
        }
    }

    return NULL;
}

static void playWalkStepSound(void)
{
    const int walkVolume = 130;
    const char* ext;
    char cmd[1024];

    if (activeWalkSoundPath == NULL)
    {
        return;
    }

    ext = strrchr(activeWalkSoundPath, '.');
    if (ext != NULL && _stricmp(ext, ".mp3") == 0)
    {
        mciSendStringA("close stepSfx", NULL, 0, NULL);
        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias stepSfx", activeWalkSoundPath);
        if (mciSendStringA(cmd, NULL, 0, NULL) == 0)
        {
            snprintf(cmd, sizeof(cmd), "setaudio stepSfx volume to %d", walkVolume);
            mciSendStringA(cmd, NULL, 0, NULL);
            mciSendStringA("play stepSfx", NULL, 0, NULL);
        }
        return;
    }

    PlaySoundA(activeWalkSoundPath, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

static void playCrowSound(void)
{
    const char* ext;
    char cmd[1024];

    if (activeCrowSoundPath == NULL)
    {
        return;
    }

    ext = strrchr(activeCrowSoundPath, '.');
    if (ext != NULL && _stricmp(ext, ".mp3") == 0)
    {
        mciSendStringA("close crowSfx", NULL, 0, NULL);
        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias crowSfx", activeCrowSoundPath);
        if (mciSendStringA(cmd, NULL, 0, NULL) == 0)
        {
            mciSendStringA("play crowSfx", NULL, 0, NULL);
        }
        return;
    }

    PlaySoundA(activeCrowSoundPath, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

static void playMonsterWalkSound(void)
{
    const char* ext;
    char cmd[1024];
    const int walkVolume = 200;
    
    if (activeMonsterWalkSoundPath == NULL)
    {
        return;
    }

    ext = strrchr(activeMonsterWalkSoundPath, '.');
    if (ext != NULL && _stricmp(ext, ".mp3") == 0)
    {
        mciSendStringA("close monsterStepSfx", NULL, 0, NULL);
        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias monsterStepSfx", activeMonsterWalkSoundPath);
        if (mciSendStringA(cmd, NULL, 0, NULL) == 0)
        {
            snprintf(cmd, sizeof(cmd), "setaudio monsterStepSfx volume to %d", walkVolume);
            mciSendStringA(cmd, NULL, 0, NULL);
            mciSendStringA("play monsterStepSfx", NULL, 0, NULL);
        }
        return;
    }

    PlaySoundA(activeMonsterWalkSoundPath, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

static void playMonsterSpotSound(void)
{
    const char* ext;
    char cmd[1024];
    char lenBuf[64];
    int totalMs;
    int halfMs;
    const int monsterVolume = 300;
    
    if (monsterSpotCooldown > 0) {
        return;
    }
    monsterSpotCooldown = 2.0f;
    
    if (activeMonsterSpotSoundPath == NULL)
    {
        return;
    }

    ext = strrchr(activeMonsterSpotSoundPath, '.');
    if (ext != NULL && _stricmp(ext, ".mp3") == 0)
    {

        mciSendStringA("close monsterSfx", NULL, 0, NULL);
        
        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias monsterSfx", activeMonsterSpotSoundPath);
        if (mciSendStringA(cmd, NULL, 0, NULL) != 0)
        {
            return;
        }
        
        snprintf(cmd, sizeof(cmd), "setaudio monsterSfx volume to %d", monsterVolume);
        mciSendStringA(cmd, NULL, 0, NULL);
        
        mciSendStringA("set monsterSfx time format milliseconds", NULL, 0, NULL);
        if (mciSendStringA("status monsterSfx length", lenBuf, sizeof(lenBuf), NULL) == 0)
        {
            totalMs = atoi(lenBuf);
            if (totalMs > 0)
            {
                halfMs = totalMs / 2;
                snprintf(cmd, sizeof(cmd), "play monsterSfx from 0 to %d", halfMs);
                mciSendStringA(cmd, NULL, 0, NULL);
                return;
            }
        }
        
        mciSendStringA("play monsterSfx", NULL, 0, NULL);
        return;
    }

    PlaySoundA(activeMonsterSpotSoundPath, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

static void startBackgroundMusic(void)
{
    int i;

    for (i = 0; i < AUDIO_CANDIDATE_COUNT; i++)
    {
        const char* path = AUDIO_CANDIDATES[i];
        const char* ext;
        DWORD attrs = GetFileAttributesA(path);

        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            continue;
        }

        ext = strrchr(path, '.');
        if (ext != NULL && _stricmp(ext, ".mp3") == 0)
        {
            if (playMp3Loop(path))
            {
                printf("Background audio: %s\n", path);
                return;
            }
            continue;
        }

        if (PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT))
        {
            printf("Background audio: %s\n", path);
            return;
        }

        fprintf(stderr, "Warning: failed to play background audio: %s\n", path);
    }

    fprintf(stderr, "Info: no background audio file found in assets/audio (mp3/wav).\n");
}

static void startMonsterAmbientSound(void)
{
    const char* ext;
    char cmd[1024];
    char lenBuf[64];
    int totalMs;
    const int ambientVolume = 200;
    
    if (activeMonsterWalkSoundPath == NULL)
    {
        return;
    }

    if (monsterProximityPlaying) {
        return;
    }

    ext = strrchr(activeMonsterWalkSoundPath, '.');
    if (ext != NULL && _stricmp(ext, ".mp3") == 0)
    {
        mciSendStringA("close monsterAmbientSfx", NULL, 0, NULL);
        
        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias monsterAmbientSfx", activeMonsterWalkSoundPath);
        if (mciSendStringA(cmd, NULL, 0, NULL) != 0)
        {
            fprintf(stderr, "Failed to open ambient sound\n");
            return;
        }
        
        snprintf(cmd, sizeof(cmd), "setaudio monsterAmbientSfx volume to %d", ambientVolume);
        mciSendStringA(cmd, NULL, 0, NULL);
        
        snprintf(cmd, sizeof(cmd), "play monsterAmbientSfx from 0 to 1000 repeat");
        if (mciSendStringA(cmd, NULL, 0, NULL) == 0)
        {
            monsterProximityPlaying = 1;
            printf("Started monster ambient sound (first 1 second loop)\n");
        }
        else
        {
            fprintf(stderr, "Failed to play ambient sound\n");
        }
        return;
    }
}

static void stopMonsterAmbientSound(void)
{
    if (monsterProximityPlaying) {
        mciSendStringA("stop monsterAmbientSfx", NULL, 0, NULL);
        mciSendStringA("close monsterAmbientSfx", NULL, 0, NULL);
        monsterProximityPlaying = 0;
        printf("Stopped monster ambient sound\n");
    }
}

static void playGateOpenFirstHalf(void)
{
    const int gateSfxSkipMs = 1500;
    int i;

    for (i = 0; i < GATE_SOUND_CANDIDATE_COUNT; i++)
    {
        const char* path = GATE_SOUND_CANDIDATES[i];
        DWORD attrs = GetFileAttributesA(path);
        char cmd[1024];
        char lenBuf[64];
        int totalMs;
        int startMs;
        int endMs;
        int halfMs;

        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            continue;
        }

        mciSendStringA("close gateSfx", NULL, 0, NULL);

        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias gateSfx", path);
        if (mciSendStringA(cmd, NULL, 0, NULL) != 0)
        {
            continue;
        }

        mciSendStringA("set gateSfx time format milliseconds", NULL, 0, NULL);
        if (mciSendStringA("status gateSfx length", lenBuf, (UINT)sizeof(lenBuf), NULL) != 0)
        {
            mciSendStringA("play gateSfx", NULL, 0, NULL);
            return;
        }

        totalMs = atoi(lenBuf);
        if (totalMs <= 0)
        {
            mciSendStringA("play gateSfx", NULL, 0, NULL);
            return;
        }

        startMs = (gateSfxSkipMs < totalMs) ? gateSfxSkipMs : 0;
        halfMs = (totalMs - startMs) / 2;
        if (halfMs < 100)
        {
            halfMs = totalMs - startMs;
        }

        endMs = startMs + halfMs;
        if (endMs > totalMs)
        {
            endMs = totalMs;
        }

        snprintf(cmd, sizeof(cmd), "play gateSfx from %d to %d", startMs, endMs);
        mciSendStringA(cmd, NULL, 0, NULL);
        return;
    }
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
    const float radius = PLAYER_COLLISION_RADIUS;

    if (isStarterGateBlockingPosition(x, y, radius))
    {
        return 0;
    }

    if (isSkeletonBlockingPosition(x, y, radius))
    {
        return 0;
    }

    return !isWall(x - radius, y - radius)
        && !isWall(x + radius, y - radius)
        && !isWall(x - radius, y + radius)
        && !isWall(x + radius, y + radius);
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
    float len;
    
    GLfloat lightPos[4];
    GLfloat lightDir[3];
    GLfloat diffuse[4];
    GLfloat ambient[4] = {0.003f, 0.003f, 0.003f, 1.0f}; 
    GLfloat globalAmbient[4] = {0.0f, 0.0f, 0.0f, 1.0f};

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

    {
        float handOffsetX = dirX * 0.3f;  
        float handOffsetY = -0.12f;       
        float handOffsetZ = dirZ * 0.3f;  

        lightPos[0] = playerX + handOffsetX;
        lightPos[1] = PLAYER_HEIGHT + handOffsetY;
        lightPos[2] = playerY + handOffsetZ;
        lightPos[3] = 1.0f;

        lightDir[0] = dirX;
        lightDir[1] = dirY - 0.08f;
        lightDir[2] = dirZ;

        len = sqrtf(lightDir[0] * lightDir[0] + lightDir[1] * lightDir[1] + lightDir[2] * lightDir[2]);
        if (len > 0.0001f)
        {
            lightDir[0] /= len;
            lightDir[1] /= len;
            lightDir[2] /= len;
        }

        if (flashlightEnabled)
        {
            float intensity = flashlightIntensity * 3.2f; 
            diffuse[0] = 0.88f * intensity;
            diffuse[1] = 0.94f * intensity;
            diffuse[2] = 1.0f * intensity;
            diffuse[3] = 1.0f;
        }
        else
        {
            ambient[0] = 0.0f;
            ambient[1] = 0.0f;
            ambient[2] = 0.0f;
            globalAmbient[0] = 0.0f;
            globalAmbient[1] = 0.0f;
            globalAmbient[2] = 0.0f;
            diffuse[0] = 0.0f;
            diffuse[1] = 0.0f;
            diffuse[2] = 0.0f;
            diffuse[3] = 1.0f;
        }

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lightDir);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 31.0f);  
        glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 45.0f);  
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.36f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.28f);

        beginWorldShader(lightPos, lightDir, diffuse, ambient,
            31.0f, 45.0f, 0.5f, 0.36f, 0.28f);
    }

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

    drawStarterGate();

    drawDeadEndSkeletonProps();

    drawEndMarker();
    
    drawMonster();
    
    endWorldShader();
    
    drawFlashlightMask();
    drawGatePrompt();

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

    if (key == 'f' || key == 'F')
    {
        flashlightEnabled = !flashlightEnabled;
        printf("Flashlight: %s\n", flashlightEnabled ? "ON" : "OFF");
    }
    else if (key == 'e' || key == 'E')
    {
        int shouldPlayGateSfx = canInteractWithStarterGate();

        tryOpenStarterGate();
        if (shouldPlayGateSfx)
        {
            playGateOpenFirstHalf();
        }
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

    currentTime = glutGet(GLUT_ELAPSED_TIME);
    if (lastTime == 0)
    {
        lastTime = currentTime;
    }
    deltaTime = (currentTime - lastTime) / 1000.0f; 

    static float debugTimer = 0;
    debugTimer += deltaTime;
    if (debugTimer > 3.0f) {
    int tx = rand() % WIDTH;
    int ty = rand() % HEIGHT;
    while (maze[ty][tx] == 1) {
        tx = rand() % WIDTH;
        ty = rand() % HEIGHT;
    }
    monsterTargetX = tx + 0.5f;
    monsterTargetY = ty + 0.5f;
    printf("FORCED new target: (%.1f, %.1f)\n", monsterTargetX, monsterTargetY);
    debugTimer = 0;
    }
    if (activeCrowSoundPath != NULL)
    {
        crowTimerSec -= deltaTime;
        if (crowTimerSec <= 0.0f)
        {
            playCrowSound();
            crowTimerSec = randomRangef(10.0f, 40.0f);
        }
    }

    updateStarterGate(deltaTime);

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

        if (activeWalkSoundPath != NULL)
        {
            int movingNow = (moveFactor > 0.12f);
            if (movingNow && wasMovingLastFrame && prevCameraBobOffset <= 0.0f && cameraBobOffset > 0.0f)
            {
                playWalkStepSound();
            }
            wasMovingLastFrame = movingNow;
        }
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
    if (!endReached && playerCellX == endTileX && playerCellY == endTileY)
    {
        endReached = true;
        printf("You reached the end of the maze!\n");
    }

    updateMonster(deltaTime);

    lastTime = currentTime;

    glutPostRedisplay();
}

void updateMonster(float deltaTime)
{
    float dx, dy, dist;
    
    monsterThinkTimer += deltaTime;
    if (monsterSpotCooldown > 0) {
        monsterSpotCooldown -= deltaTime;
    }
    
    monsterProximityCheckTimer += deltaTime;
    if (monsterProximityCheckTimer > 0.2f) {
        float playerDx = playerX - monsterX;
        float playerDy = playerY - monsterY;
        float distToPlayer = sqrtf(playerDx*playerDx + playerDy*playerDy);
        
        const float PROXIMITY_THRESHOLD = 5.0f;
        
        if (distToPlayer < PROXIMITY_THRESHOLD) {
            if (!monsterProximityPlaying) {
                startMonsterAmbientSound();
            }
            monsterProximityTimer = 0;
        } else {
            monsterProximityTimer += monsterProximityCheckTimer;
            if (monsterProximityPlaying && monsterProximityTimer > 1.0f) {
                stopMonsterAmbientSound();
            }
        }
        
        monsterProximityCheckTimer = 0;
    }
    
    int canSee = monsterCanSeePlayer();
    
    if (canSee) {
        if (monsterState != 1) {
            playMonsterSpotSound();
            printf("*** MONSTER SPOTTED YOU! ***\n");
        }
        monsterState = 1;
        monsterTargetX = playerX;
        monsterTargetY = playerY;
        monsterThinkTimer = 0;
    }
    
    if (monsterState == 1) {
        if (monsterThinkTimer > 5.0f) {
            monsterState = 0;
            monsterPickRandomTarget();
            monsterThinkTimer = 0;
            printf("Monster lost interest\n");
        } else {
            monsterTargetX = playerX;
            monsterTargetY = playerY;
        }
    } else {
        if (monsterThinkTimer > 2.0f) {
            monsterPickRandomTarget();
            monsterThinkTimer = 0;
        }
    }
    
    dx = monsterTargetX - monsterX;
    dy = monsterTargetY - monsterY;
    dist = sqrtf(dx*dx + dy*dy);
    
    float oldX = monsterX;
    float oldY = monsterY;
    
    if (dist > 0.3f) {
        float moveX = 0.0f;
        float moveY = 0.0f;
        
        if (dist > 0) {
            moveX = (dx / dist) * monsterSpeed * deltaTime * 60.0f;
            moveY = (dy / dist) * monsterSpeed * deltaTime * 60.0f;
        }
        
        const float monsterRadius = PLAYER_COLLISION_RADIUS;
        
        float newX = monsterX + moveX;
        if (!isWall(newX - monsterRadius, monsterY - monsterRadius) &&
            !isWall(newX + monsterRadius, monsterY - monsterRadius) &&
            !isWall(newX - monsterRadius, monsterY + monsterRadius) &&
            !isWall(newX + monsterRadius, monsterY + monsterRadius)) {
            monsterX = newX;
        }
        
        float newY = monsterY + moveY;
        if (!isWall(monsterX - monsterRadius, newY - monsterRadius) &&
            !isWall(monsterX + monsterRadius, newY - monsterRadius) &&
            !isWall(monsterX - monsterRadius, newY + monsterRadius) &&
            !isWall(monsterX + monsterRadius, newY + monsterRadius)) {
            monsterY = newY;
        }
        
        if (fabsf(moveX) > 0.001f || fabsf(moveY) > 0.001f) {
            monsterAngle = atan2f(moveY, moveX);
        }
    } else {
        if (monsterState == 0) {
            monsterPickRandomTarget();
        }
    }
    
    float movedDist = sqrtf((monsterX - oldX)*(monsterX - oldX) + (monsterY - oldY)*(monsterY - oldY));
    int isMoving = (movedDist > 0.01f);
    
    if (isMoving) {
    monsterWalkTimer += deltaTime;
    if (monsterWalkTimer > 0.25f) {
        playMonsterWalkSound();
        monsterWalkTimer = 0;
    }
    } else {
        monsterWalkTimer = 0;
    }
    
    monsterWasMovingLastFrame = isMoving;
}

int monsterCanSeePlayer()
{
    float dx = playerX - monsterX;
    float dy = playerY - monsterY;
    float dist = sqrtf(dx*dx + dy*dy);
    
    if(dist > 8.0f)
        return 0;
    
    float steps = dist / 0.3f;
    float stepX = dx / steps;
    float stepY = dy / steps;
    
    for(int i = 1; i < (int)steps; i++) {
        float checkX = monsterX + stepX * i;
        float checkY = monsterY + stepY * i;
        if(isWall(checkX, checkY)) {
            return 0;
        }
    }
    
    return 1;
}

void monsterPickRandomTarget()
{
    int tx, ty;
    do {
        tx = rand() % WIDTH;
        ty = rand() % HEIGHT;
    } while(maze[ty][tx] == 1);
    monsterTargetX = tx + 0.5f;
    monsterTargetY = ty + 0.5f;
}

void drawMonster()
{
    float dx = playerX - monsterX;
    float dy = playerY - monsterY;
    float distToPlayer = sqrtf(dx*dx + dy*dy);
    
    glPushMatrix();
    glTranslatef(monsterX, 0.0f, monsterY);
    
    if (zombieTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, zombieTexture);
        setWorldShaderUseTexture(1);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        setWorldShaderUseTexture(0);
    }
    
    if (hasZombieMesh()) {
        float faceX = cosf(monsterAngle);
        float faceZ = sinf(monsterAngle);
        drawZombieAt(0, 0, faceX, faceZ, 0.9f);
    } else {
        glRotatef(monsterAngle * 180.0f / M_PI, 0, 1, 0);
        glPushMatrix();
        glTranslatef(0, 0.9f, 0);
        glutSolidCube(0.8);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, 1.5f, 0);
        glutSolidSphere(0.3, 16, 16);
        glPopMatrix();
    }
    
    if (zombieTexture != 0) {
        glDisable(GL_TEXTURE_2D);
    }
    setWorldShaderUseTexture(0);
    glPopMatrix();
    
    static int frameCount = 0;
    frameCount++;
    if (frameCount < 300) {
        printf("ZOMBIE at (%.2f, %.2f), angle: %.1f, dist: %.2f\n", 
               monsterX, monsterY, monsterAngle * 180/M_PI, distToPlayer);
    }
}

void findSafeMonsterSpawn()
{
    int attempts = 0;
    int tx, ty;
    float safeX, safeY;
    const float monsterRadius = PLAYER_COLLISION_RADIUS;
    
    do {
        tx = rand() % WIDTH;
        ty = rand() % HEIGHT;
        safeX = tx + 0.5f;
        safeY = ty + 0.5f;
        attempts++;
        
        float dx = safeX - playerX;
        float dy = safeY - playerY;
        float distToPlayer = sqrtf(dx*dx + dy*dy);
        
        if (!isWall(safeX, safeY) && distToPlayer > 5.0f) {
            if (!isWall(safeX - monsterRadius, safeY - monsterRadius) &&
                !isWall(safeX + monsterRadius, safeY - monsterRadius) &&
                !isWall(safeX - monsterRadius, safeY + monsterRadius) &&
                !isWall(safeX + monsterRadius, safeY + monsterRadius)) {
                
                monsterX = safeX;
                monsterY = safeY;
                monsterPickRandomTarget();
                printf("Monster spawned at (%.1f, %.1f), distance from player: %.1f\n", 
                       monsterX, monsterY, distToPlayer);
                return;
            }
        }
    } while (attempts < 500);
        
    for (ty = 0; ty < HEIGHT; ty++) {
        for (tx = 0; tx < WIDTH; tx++) {
            if (maze[ty][tx] == 0) {
                safeX = tx + 0.5f;
                safeY = ty + 0.5f;
                
                float dx = safeX - playerX;
                float dy = safeY - playerY;
                if (sqrtf(dx*dx + dy*dy) > 2.0f) {
                    monsterX = safeX;
                    monsterY = safeY;
                    monsterPickRandomTarget();
                    printf("Monster spawned at fallback position (%.1f, %.1f)\n", monsterX, monsterY);
                    return;
                }
            }
        }
    }
    
    monsterX = 10.5f;
    monsterY = 10.5f;
}

void init(void)
{
    GLfloat globalAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f}; 
    MazeGenResult mazeResult;

    
    srand((unsigned int)time(NULL));
    generateMazeDFS(&maze[0][0], WIDTH, HEIGHT, 1, 1, START_ROOM_HALF_SIZE, &mazeResult);
    playerX = (float)mazeResult.startX + 0.5f;
    playerY = (float)mazeResult.startY + 0.5f;
    findSafeMonsterSpawn();
    startTileX = mazeResult.startX;
    startTileY = mazeResult.startY;
    endTileX = mazeResult.endX;
    endTileY = mazeResult.endY;
    endReached = false;
    setupStarterGate(mazeResult.startX, mazeResult.startY, START_ROOM_HALF_SIZE);
    rebuildSkeletonInstances();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    initWorldShaderProgram();
    
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_DIFFUSE);
    
    glutSetCursor(GLUT_CURSOR_NONE);

    wallTexture = loadTexture(WALL_TEXTURE_PATH, 0);
    floorTexture = loadTexture(FLOOR_TEXTURE_PATH, 1);
    gateTexture = loadTexture(GATE_TEXTURE_PATH, 0);
    skeletonTexture = loadTexture(SKELETON_TEXTURE_PATH, 0);
    zombieTexture = loadTexture(ZOMBIE_TEXTURE_PATH, 0);
    loadGateMesh(GATE_OBJ_PATH);
    loadSkeletonMesh(SKELETON_OBJ_PATH);
    loadZombieMesh(ZOMBIE_OBJ_PATH);
    loadSkyTexture(SKY_TEXTURE_PATH);

    activeWalkSoundPath = findFirstExistingAudio(WALK_SOUND_CANDIDATES, WALK_SOUND_CANDIDATE_COUNT);

    activeCrowSoundPath = findFirstExistingAudio(CROW_SOUND_CANDIDATES, CROW_SOUND_CANDIDATE_COUNT);
    if (activeCrowSoundPath != NULL)
    {
        crowTimerSec = randomRangef(10.0f, 40.0f);
    }

    activeMonsterSpotSoundPath = findFirstExistingAudio(MONSTER_SPOT_SOUND_CANDIDATES, MONSTER_SPOT_SOUND_CANDIDATE_COUNT);

    activeMonsterWalkSoundPath = findFirstExistingAudio(MONSTER_WALK_SOUND_CANDIDATES, MONSTER_WALK_SOUND_CANDIDATE_COUNT);

    startBackgroundMusic();
}

int main(int argc, char** argv)
{
    atexit(stopBackgroundMusic);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Maze - Labyrinthine Style Flashlight");
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