#ifndef MAZE_GENERATOR_H
#define MAZE_GENERATOR_H

typedef struct MazeGenResult {
    int startX;
    int startY;
    int endX;
    int endY;
} MazeGenResult;

void generateMazeDFS(int *maze, int width, int height,
                     int requestedStartX, int requestedStartY,
                     int roomHalfSize, MazeGenResult *result);

#endif
