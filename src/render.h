#ifndef RENDER_H
#define RENDER_H

#include <SDL_opengl.h>

#include "game.h"

int initBitmapFont(void);
void shutdownBitmapFont(void);
GLuint loadTexture(const char* path, int isFloorTexture);
void reshape(int width, int height);
void display(void);

#endif