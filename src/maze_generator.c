#include "maze_generator.h"

#include <stdlib.h>

typedef struct Cell {
    int x;
    int y;
} Cell;

typedef struct RoomOpening {
    int roomX;
    int roomY;
    int outsideX;
    int outsideY;
} RoomOpening;

typedef struct SearchCell {
    int x;
    int y;
} SearchCell;

static int toIndex(int x, int y, int width)
{
    return y * width + x;
}

static int clampi(int value, int minValue, int maxValue)
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

static int forceOddInRange(int value, int minValue, int maxValue)
{
    int result = clampi(value, minValue, maxValue);

    if ((result % 2) == 0)
    {
        if (result < maxValue)
        {
            result++;
        }
        else
        {
            result--;
        }
    }

    return clampi(result, minValue, maxValue);
}

static void carveRoom(int *maze, int width, int height, int centerX, int centerY, int halfSize)
{
    int x;
    int y;

    for (y = centerY - halfSize; y <= centerY + halfSize; y++)
    {
        if (y <= 0 || y >= height - 1)
        {
            continue;
        }

        for (x = centerX - halfSize; x <= centerX + halfSize; x++)
        {
            if (x <= 0 || x >= width - 1)
            {
                continue;
            }

            maze[toIndex(x, y, width)] = 0;
        }
    }
}

static int isInsideRoom(int x, int y, int centerX, int centerY, int halfSize)
{
    return x >= centerX - halfSize && x <= centerX + halfSize
        && y >= centerY - halfSize && y <= centerY + halfSize;
}

static int isAdjacentToRoom(int x, int y, int centerX, int centerY, int halfSize)
{
    return isInsideRoom(x - 1, y, centerX, centerY, halfSize)
        || isInsideRoom(x + 1, y, centerX, centerY, halfSize)
        || isInsideRoom(x, y - 1, centerX, centerY, halfSize)
        || isInsideRoom(x, y + 1, centerX, centerY, halfSize);
}

static int isProtectedRoomApproach(int x, int y,
    int startRoomX, int startRoomY, int endRoomX, int endRoomY, int roomHalfSize,
    RoomOpening startOpening, RoomOpening endOpening)
{
    if (x == startOpening.outsideX && y == startOpening.outsideY)
    {
        return 0;
    }
    if (x == endOpening.outsideX && y == endOpening.outsideY)
    {
        return 0;
    }

    return isAdjacentToRoom(x, y, startRoomX, startRoomY, roomHalfSize)
        || isAdjacentToRoom(x, y, endRoomX, endRoomY, roomHalfSize);
}

static int isPathReachable(const int *maze, int width, int height,
    int startX, int startY, int endX, int endY)
{
    unsigned char *visited;
    SearchCell *queue;
    int head = 0;
    int tail = 0;
    int reachable = 0;
    int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    if (maze[toIndex(startX, startY, width)] != 0 || maze[toIndex(endX, endY, width)] != 0)
    {
        return 0;
    }

    visited = (unsigned char *)calloc((size_t)(width * height), sizeof(unsigned char));
    queue = (SearchCell *)malloc((size_t)(width * height) * sizeof(SearchCell));
    if (visited == 0 || queue == 0)
    {
        free(visited);
        free(queue);
        return 0;
    }

    visited[toIndex(startX, startY, width)] = 1;
    queue[tail].x = startX;
    queue[tail].y = startY;
    tail++;

    while (head < tail)
    {
        SearchCell current = queue[head++];
        int i;

        if (current.x == endX && current.y == endY)
        {
            reachable = 1;
            break;
        }

        for (i = 0; i < 4; i++)
        {
            int nx = current.x + dirs[i][0];
            int ny = current.y + dirs[i][1];

            if (nx <= 0 || nx >= width - 1 || ny <= 0 || ny >= height - 1)
            {
                continue;
            }
            if (maze[toIndex(nx, ny, width)] != 0 || visited[toIndex(nx, ny, width)])
            {
                continue;
            }

            visited[toIndex(nx, ny, width)] = 1;
            queue[tail].x = nx;
            queue[tail].y = ny;
            tail++;
        }
    }

    free(visited);
    free(queue);
    return reachable;
}

static void carvePathBetweenOpenings(int *maze, int width, int height,
    RoomOpening startOpening, RoomOpening endOpening,
    int startRoomX, int startRoomY, int endRoomX, int endRoomY, int roomHalfSize)
{
    int cellCount = width * height;
    int *distances = (int *)malloc((size_t)cellCount * sizeof(int));
    int *parents = (int *)malloc((size_t)cellCount * sizeof(int));
    unsigned char *used = (unsigned char *)calloc((size_t)cellCount, sizeof(unsigned char));
    int startIndex = toIndex(startOpening.outsideX, startOpening.outsideY, width);
    int endIndex = toIndex(endOpening.outsideX, endOpening.outsideY, width);
    int currentIndex;
    int i;

    if (distances == 0 || parents == 0 || used == 0)
    {
        free(distances);
        free(parents);
        free(used);
        return;
    }

    for (i = 0; i < cellCount; i++)
    {
        distances[i] = width * height * 4;
        parents[i] = -1;
    }
    distances[startIndex] = 0;

    while (1)
    {
        int bestDistance = width * height * 4;
        int x;
        int y;

        currentIndex = -1;
        for (i = 0; i < cellCount; i++)
        {
            if (!used[i] && distances[i] < bestDistance)
            {
                bestDistance = distances[i];
                currentIndex = i;
            }
        }

        if (currentIndex < 0 || currentIndex == endIndex)
        {
            break;
        }

        used[currentIndex] = 1;
        x = currentIndex % width;
        y = currentIndex / width;

        for (i = 0; i < 4; i++)
        {
            int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            int nx = x + dirs[i][0];
            int ny = y + dirs[i][1];
            int nextIndex;
            int stepCost;

            if (nx <= 0 || nx >= width - 1 || ny <= 0 || ny >= height - 1)
            {
                continue;
            }
            if (isInsideRoom(nx, ny, startRoomX, startRoomY, roomHalfSize)
                || isInsideRoom(nx, ny, endRoomX, endRoomY, roomHalfSize))
            {
                continue;
            }
            if (isProtectedRoomApproach(nx, ny,
                startRoomX, startRoomY, endRoomX, endRoomY, roomHalfSize,
                startOpening, endOpening))
            {
                continue;
            }

            nextIndex = toIndex(nx, ny, width);
            stepCost = (maze[nextIndex] == 0) ? 0 : 1;
            if (distances[currentIndex] + stepCost < distances[nextIndex])
            {
                distances[nextIndex] = distances[currentIndex] + stepCost;
                parents[nextIndex] = currentIndex;
            }
        }
    }

    if (parents[endIndex] >= 0 || endIndex == startIndex)
    {
        currentIndex = endIndex;
        while (currentIndex >= 0)
        {
            maze[currentIndex] = 0;
            if (currentIndex == startIndex)
            {
                break;
            }
            currentIndex = parents[currentIndex];
        }
    }

    free(distances);
    free(parents);
    free(used);
}

static RoomOpening enforceSingleRoomOpening(int *maze, int width, int height,
    int centerX, int centerY, int halfSize, int targetX, int targetY)
{
    RoomOpening openings[128];
    RoomOpening chosenOpening;
    int openingCount = 0;
    int x;
    int y;
    int i;
    int chosen = -1;
    int left = centerX - halfSize;
    int right = centerX + halfSize;
    int top = centerY - halfSize;
    int bottom = centerY + halfSize;
    int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

    chosenOpening.roomX = centerX;
    chosenOpening.roomY = centerY;
    chosenOpening.outsideX = centerX;
    chosenOpening.outsideY = centerY;

    for (y = top; y <= bottom; y++)
    {
        for (x = left; x <= right; x++)
        {
            if (!(x == left || x == right || y == top || y == bottom))
            {
                continue;
            }

            for (i = 0; i < 4; i++)
            {
                int nx = x + dirs[i][0];
                int ny = y + dirs[i][1];

                if (nx <= 0 || nx >= width - 1 || ny <= 0 || ny >= height - 1)
                {
                    continue;
                }
                if (isInsideRoom(nx, ny, centerX, centerY, halfSize))
                {
                    continue;
                }
                if (maze[toIndex(nx, ny, width)] != 0)
                {
                    continue;
                }

                if (openingCount < (int)(sizeof(openings) / sizeof(openings[0])))
                {
                    openings[openingCount].roomX = x;
                    openings[openingCount].roomY = y;
                    openings[openingCount].outsideX = nx;
                    openings[openingCount].outsideY = ny;
                    openingCount++;
                }
            }
        }
    }

    if (openingCount == 0)
    {
        int dx = targetX - centerX;
        int dy = targetY - centerY;
        int doorRoomX = centerX;
        int doorRoomY = centerY;
        int doorOutsideX = centerX;
        int doorOutsideY = centerY;

        if (abs(dx) >= abs(dy))
        {
            if (dx >= 0)
            {
                doorRoomX = right;
                doorOutsideX = right + 1;
            }
            else
            {
                doorRoomX = left;
                doorOutsideX = left - 1;
            }
            doorRoomY = clampi(centerY, top, bottom);
            doorOutsideY = doorRoomY;
        }
        else
        {
            if (dy >= 0)
            {
                doorRoomY = bottom;
                doorOutsideY = bottom + 1;
            }
            else
            {
                doorRoomY = top;
                doorOutsideY = top - 1;
            }
            doorRoomX = clampi(centerX, left, right);
            doorOutsideX = doorRoomX;
        }

        if (doorOutsideX > 0 && doorOutsideX < width - 1 && doorOutsideY > 0 && doorOutsideY < height - 1)
        {
            maze[toIndex(doorRoomX, doorRoomY, width)] = 0;
            maze[toIndex(doorOutsideX, doorOutsideY, width)] = 0;
            chosenOpening.roomX = doorRoomX;
            chosenOpening.roomY = doorRoomY;
            chosenOpening.outsideX = doorOutsideX;
            chosenOpening.outsideY = doorOutsideY;
        }
        return chosenOpening;
    }

    for (i = 0; i < openingCount; i++)
    {
        int d = abs(openings[i].outsideX - targetX) + abs(openings[i].outsideY - targetY);
        if (chosen < 0)
        {
            chosen = i;
        }
        else
        {
            int bestD = abs(openings[chosen].outsideX - targetX) + abs(openings[chosen].outsideY - targetY);
            if (d < bestD)
            {
                chosen = i;
            }
        }
    }

    for (i = 0; i < openingCount; i++)
    {
        if (i == chosen)
        {
            chosenOpening = openings[i];
            continue;
        }
        maze[toIndex(openings[i].outsideX, openings[i].outsideY, width)] = 1;
    }

    return chosenOpening;
}

void generateMazeDFS(int *maze, int width, int height,
                     int requestedStartX, int requestedStartY,
                     int roomHalfSize, MazeGenResult *result)
{
    int i;
    int x;
    int y;
    int stackTop = 0;
    int maxDistance = -1;
    int startX;
    int startY;
    int endX;
    int endY;
    int dirs[4][2] = {{0, -2}, {0, 2}, {-2, 0}, {2, 0}};
    int shuffledDirs[4];
    int minX;
    int minY;
    int maxX;
    int maxY;
    RoomOpening startOpening;
    RoomOpening endOpening;
    unsigned char *visited;
    Cell *stack;

    if (maze == 0 || result == 0 || width < 5 || height < 5)
    {
        return;
    }

    if (roomHalfSize < 1)
    {
        roomHalfSize = 1;
    }

    minX = 1 + roomHalfSize;
    minY = 1 + roomHalfSize;
    maxX = width - 2 - roomHalfSize;
    maxY = height - 2 - roomHalfSize;

    if (maxX < minX)
    {
        minX = 1;
        maxX = width - 2;
    }
    if (maxY < minY)
    {
        minY = 1;
        maxY = height - 2;
    }

    startX = forceOddInRange(requestedStartX, minX, maxX);
    startY = forceOddInRange(requestedStartY, minY, maxY);

    visited = (unsigned char *)calloc((size_t)(width * height), sizeof(unsigned char));
    stack = (Cell *)malloc((size_t)(width * height) * sizeof(Cell));
    if (visited == 0 || stack == 0)
    {
        free(visited);
        free(stack);
        return;
    }

    for (i = 0; i < width * height; i++)
    {
        maze[i] = 1;
    }

    maze[toIndex(startX, startY, width)] = 0;
    visited[toIndex(startX, startY, width)] = 1;
    stack[stackTop].x = startX;
    stack[stackTop].y = startY;
    stackTop++;

    while (stackTop > 0)
    {
        Cell current = stack[stackTop - 1];
        int found = 0;

        for (i = 0; i < 4; i++)
        {
            shuffledDirs[i] = i;
        }
        for (i = 0; i < 4; i++)
        {
            int r = rand() % 4;
            int temp = shuffledDirs[i];
            shuffledDirs[i] = shuffledDirs[r];
            shuffledDirs[r] = temp;
        }

        for (i = 0; i < 4; i++)
        {
            int dirIndex = shuffledDirs[i];
            int nx = current.x + dirs[dirIndex][0];
            int ny = current.y + dirs[dirIndex][1];

            if (nx > 0 && nx < width - 1 && ny > 0 && ny < height - 1
                && !visited[toIndex(nx, ny, width)])
            {
                int midX = current.x + dirs[dirIndex][0] / 2;
                int midY = current.y + dirs[dirIndex][1] / 2;

                maze[toIndex(nx, ny, width)] = 0;
                maze[toIndex(midX, midY, width)] = 0;
                visited[toIndex(nx, ny, width)] = 1;

                stack[stackTop].x = nx;
                stack[stackTop].y = ny;
                stackTop++;
                found = 1;
                break;
            }
        }

        if (!found)
        {
            stackTop--;
        }
    }

    carveRoom(maze, width, height, startX, startY, roomHalfSize);

    endX = startX;
    endY = startY;

    for (y = minY; y <= maxY; y++)
    {
        for (x = minX; x <= maxX; x++)
        {
            if (maze[toIndex(x, y, width)] == 0)
            {
                int distance = abs(y - startY) + abs(x - startX);
                int neighbors = 0;

                if (maze[toIndex(x, y - 1, width)] == 0) neighbors++;
                if (maze[toIndex(x, y + 1, width)] == 0) neighbors++;
                if (maze[toIndex(x - 1, y, width)] == 0) neighbors++;
                if (maze[toIndex(x + 1, y, width)] == 0) neighbors++;

                if (neighbors >= 2 && distance > maxDistance)
                {
                    maxDistance = distance;
                    endX = x;
                    endY = y;
                }
            }
        }
    }

    if (maxDistance < 0)
    {
        for (y = minY; y <= maxY; y++)
        {
            for (x = minX; x <= maxX; x++)
            {
                if (maze[toIndex(x, y, width)] == 0)
                {
                    int distance = abs(y - startY) + abs(x - startX);
                    if (distance > maxDistance)
                    {
                        maxDistance = distance;
                        endX = x;
                        endY = y;
                    }
                }
            }
        }
    }

    endX = forceOddInRange(endX, minX, maxX);
    endY = forceOddInRange(endY, minY, maxY);
    carveRoom(maze, width, height, endX, endY, roomHalfSize);

    startOpening = enforceSingleRoomOpening(maze, width, height, startX, startY, roomHalfSize, endX, endY);
    endOpening = enforceSingleRoomOpening(maze, width, height, endX, endY, roomHalfSize, startX, startY);

    if (!isPathReachable(maze, width, height, startX, startY, endX, endY))
    {
        carvePathBetweenOpenings(maze, width, height,
            startOpening, endOpening,
            startX, startY, endX, endY, roomHalfSize);
    }

    result->startX = startX;
    result->startY = startY;
    result->endX = endX;
    result->endY = endY;

    free(visited);
    free(stack);
}
