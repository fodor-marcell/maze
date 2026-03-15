#ifndef GATE_H
#define GATE_H

#include "maze_shared.h"

typedef struct EntranceGate {
    int active;
    float hingeX;
    float hingeZ;
    float width;
    float height;
    float thickness;
    float yawDeg;
    float openAngleDeg;
    float currentAngleDeg;
    float targetAngleDeg;
    float centerX;
    float centerZ;
    float normalX;
    float normalZ;
    int isOpen;
} EntranceGate;

void setupStarterGate(int roomCenterX, int roomCenterY, int roomHalfSize);
int isStarterGateBlockingPosition(float x, float y, float radius);
int canInteractWithStarterGate(void);
void tryOpenStarterGate(void);
void updateStarterGate(float deltaTime);
void loadGateMesh(const char *path);
void drawStarterGate(void);
void loadSkeletonMesh(const char *path);
int hasSkeletonMesh(void);
void drawSkeletonMeshAt(float worldX, float worldZ, float faceX, float faceZ, float targetHeight);
void loadZombieMesh(const char *path);
void drawZombieAt(float worldX, float worldZ, float faceX, float faceZ, float targetHeight);
int hasZombieMesh(void);

#endif