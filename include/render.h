#ifndef RENDER_H
#define RENDER_H

#include "maze_shared.h"

GLuint loadTexture(const char *path, int repeatTexture);
void loadSkyTexture(const char *path);
void drawFlashlightMask(void);
void drawFloorAndCeiling(void);
void drawEndMarker(void);
void drawSkybox(float cameraX, float cameraY, float cameraZ);
void drawWall(float x, float y);
void drawGatePrompt(void);

#endif