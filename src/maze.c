#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "maze_shared.h"
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


float flashlightIntensity = 1.0f;

GLuint wallTexture = 0;
GLuint floorTexture = 0;
GLuint skyTexture = 0;
GLuint gateTexture = 0;

const char* WALL_TEXTURE_PATH = "assets/textures/wall.png";
const char* FLOOR_TEXTURE_PATH = "assets/textures/floor.jpg";
const char* SKY_TEXTURE_PATH = "assets/textures/sky.jpg";
const char* GATE_TEXTURE_PATH = "assets/textures/wall.png";
const char* GATE_OBJ_PATH     = "assets/textures/gate.obj";

int endTileX = 14;
int endTileY = 10;

int maze[HEIGHT][WIDTH];

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

    if (isStarterGateBlockingPosition(x, y, radius))
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
    GLfloat ambient[4] = {0.02f, 0.02f, 0.02f, 1.0f}; 
    GLfloat globalAmbient[4] = {0.05f, 0.05f, 0.05f, 1.0f};

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

    drawEndMarker();

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
        tryOpenStarterGate();
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
    int currentTime;
    float deltaTime;

    currentTime = glutGet(GLUT_ELAPSED_TIME);
    if (lastTime == 0)
    {
        lastTime = currentTime;
    }
    deltaTime = (currentTime - lastTime) / 1000.0f; 

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
    }
    else
    {
        float settle = clampf(deltaTime * 9.0f, 0.0f, 1.0f);
        cameraBobOffset += (0.0f - cameraBobOffset) * settle;
    }

    playerCellX = (int)floorf(playerX);
    playerCellY = (int)floorf(playerY);
    if (!endReached && playerCellX == endTileX && playerCellY == endTileY)
    {
        endReached = true;
        printf("You reached the end of the maze!\n");
    }

    lastTime = currentTime;

    glutPostRedisplay();
}

void init(void)
{
    GLfloat globalAmbient[] = {0.05f, 0.05f, 0.05f, 1.0f}; 
    MazeGenResult mazeResult;

    
    srand((unsigned int)time(NULL));
    generateMazeDFS(&maze[0][0], WIDTH, HEIGHT, 1, 1, START_ROOM_HALF_SIZE, &mazeResult);
    playerX = (float)mazeResult.startX + 0.5f;
    playerY = (float)mazeResult.startY + 0.5f;
    endTileX = mazeResult.endX;
    endTileY = mazeResult.endY;
    endReached = false;
    setupStarterGate(mazeResult.startX, mazeResult.startY, START_ROOM_HALF_SIZE);

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
    loadGateMesh(GATE_OBJ_PATH);
    loadSkyTexture(SKY_TEXTURE_PATH);

    if (wallTexture == 0)
    {
        fprintf(stderr, "Warning: failed to load wall texture: %s\n", WALL_TEXTURE_PATH);
    }
    if (floorTexture == 0)
    {
        fprintf(stderr, "Warning: failed to load floor texture: %s\n", FLOOR_TEXTURE_PATH);
    }
    if (skyTexture == 0)
    {
        fprintf(stderr, "Warning: failed to load sky texture: %s\n", SKY_TEXTURE_PATH);
    }
    if (gateTexture == 0)
    {
        fprintf(stderr, "Warning: failed to load gate texture: %s\n", GATE_TEXTURE_PATH);
    }
}

int main(int argc, char** argv)
{
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
