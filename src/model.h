#ifndef MODEL_H
#define MODEL_H

#include "game.h"

int loadOBJ(const char* path, Model* model);
void freeModel(Model* model);

#endif