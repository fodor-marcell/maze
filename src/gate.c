#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gate.h"
#include "glshader.h"

typedef struct ObjMesh {
    float *verts;
    float *uvs;
    float *normals;
    int count;
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
} ObjMesh;

static EntranceGate starterGate = {0};
static ObjMesh gateMesh = {0};
static ObjMesh skeletonMesh = {0};
static ObjMesh zombieMesh = {0};

static int findRoomOpening(int centerX, int centerY, int roomHalfSize,
    int *roomX, int *roomY, int *outsideX, int *outsideY)
{
    int left = centerX - roomHalfSize;
    int right = centerX + roomHalfSize;
    int top = centerY - roomHalfSize;
    int bottom = centerY + roomHalfSize;
    int x;
    int y;

    if (top > 0)
    {
        for (x = left; x <= right; x++)
        {
            if (maze[top - 1][x] == 0)
            {
                *roomX = x;
                *roomY = top;
                *outsideX = x;
                *outsideY = top - 1;
                return 1;
            }
        }
    }

    if (bottom < HEIGHT - 1)
    {
        for (x = left; x <= right; x++)
        {
            if (maze[bottom + 1][x] == 0)
            {
                *roomX = x;
                *roomY = bottom;
                *outsideX = x;
                *outsideY = bottom + 1;
                return 1;
            }
        }
    }

    if (left > 0)
    {
        for (y = top; y <= bottom; y++)
        {
            if (maze[y][left - 1] == 0)
            {
                *roomX = left;
                *roomY = y;
                *outsideX = left - 1;
                *outsideY = y;
                return 1;
            }
        }
    }

    if (right < WIDTH - 1)
    {
        for (y = top; y <= bottom; y++)
        {
            if (maze[y][right + 1] == 0)
            {
                *roomX = right;
                *roomY = y;
                *outsideX = right + 1;
                *outsideY = y;
                return 1;
            }
        }
    }

    return 0;
}

void setupStarterGate(int roomCenterX, int roomCenterY, int roomHalfSize)
{
    int roomX;
    int roomY;
    int outsideX;
    int outsideY;
    int dirX;
    int dirY;
    float openingCenterX;
    float openingCenterZ;
    float yawRadians;
    float rightX;
    float rightZ;

    starterGate.active = 0;

    if (!findRoomOpening(roomCenterX, roomCenterY, roomHalfSize, &roomX, &roomY, &outsideX, &outsideY))
    {
        return;
    }

    dirX = outsideX - roomX;
    dirY = outsideY - roomY;

    starterGate.width = (WALL_HEIGHT / 2.9208f) * 1.4703f;
    starterGate.height = WALL_HEIGHT - 0.14f;
    starterGate.thickness = 0.06f;
    starterGate.openAngleDeg = 80.0f;
    starterGate.currentAngleDeg = 0.0f;
    starterGate.targetAngleDeg = 0.0f;
    starterGate.isOpen = 0;

    if (dirX != 0)
    {
        openingCenterX = (float)roomX + (dirX > 0 ? 1.0f : 0.0f);
        openingCenterZ = (float)roomY + 0.5f;
        starterGate.yawDeg = (dirX > 0) ? 90.0f : -90.0f;
    }
    else
    {
        openingCenterX = (float)roomX + 0.5f;
        openingCenterZ = (float)roomY + (dirY > 0 ? 1.0f : 0.0f);
        starterGate.yawDeg = (dirY > 0) ? 0.0f : 180.0f;
    }

    starterGate.normalX = (float)dirX;
    starterGate.normalZ = (float)dirY;
    starterGate.centerX = openingCenterX * TILE + starterGate.normalX * 0.12f;
    starterGate.centerZ = openingCenterZ * TILE;

    yawRadians = starterGate.yawDeg * ((float)M_PI / 180.0f);
    rightX = cosf(yawRadians);
    rightZ = -sinf(yawRadians);
    starterGate.hingeX = starterGate.centerX - rightX * starterGate.width * 0.5f;
    starterGate.hingeZ = starterGate.centerZ - rightZ * starterGate.width * 0.5f;
    starterGate.active = 1;
}

int isStarterGateBlockingPosition(float x, float y, float radius)
{
    float localX;
    float localZ;
    float radians;
    float cosYaw;
    float sinYaw;
    float halfDepth;

    if (!starterGate.active)
    {
        return 0;
    }

    if (starterGate.currentAngleDeg >= starterGate.openAngleDeg - 4.0f)
    {
        return 0;
    }

    localX = x - starterGate.centerX;
    localZ = y - starterGate.centerZ;
    radians = -starterGate.yawDeg * ((float)M_PI / 180.0f);
    cosYaw = cosf(radians);
    sinYaw = sinf(radians);

    {
        float rotatedX = localX * cosYaw - localZ * sinYaw;
        float rotatedZ = localX * sinYaw + localZ * cosYaw;
        halfDepth = starterGate.thickness + radius + 0.03f;

        if (fabsf(rotatedZ) <= halfDepth && fabsf(rotatedX) <= starterGate.width * 0.5f + radius)
        {
            return 1;
        }
    }

    return 0;
}

int canInteractWithStarterGate(void)
{
    float toGateX;
    float toGateZ;
    float distance;
    float lookX;
    float lookZ;
    float facing;

    if (!starterGate.active || starterGate.isOpen)
    {
        return 0;
    }

    toGateX = starterGate.centerX - playerX;
    toGateZ = starterGate.centerZ - playerY;
    distance = sqrtf(toGateX * toGateX + toGateZ * toGateZ);
    if (distance > 1.35f || distance < 0.001f)
    {
        return 0;
    }

    toGateX /= distance;
    toGateZ /= distance;
    lookX = cosf(playerAngle);
    lookZ = sinf(playerAngle);
    facing = toGateX * lookX + toGateZ * lookZ;

    return facing > 0.78f;
}

void tryOpenStarterGate(void)
{
    if (!canInteractWithStarterGate())
    {
        return;
    }

    starterGate.targetAngleDeg = starterGate.openAngleDeg;
    starterGate.isOpen = 1;
}

void updateStarterGate(float deltaTime)
{
    const float openSpeedDeg = 42.5f;

    if (!starterGate.active)
    {
        return;
    }

    if (starterGate.currentAngleDeg < starterGate.targetAngleDeg)
    {
        starterGate.currentAngleDeg += openSpeedDeg * deltaTime;
        if (starterGate.currentAngleDeg > starterGate.targetAngleDeg)
        {
            starterGate.currentAngleDeg = starterGate.targetAngleDeg;
        }
    }
}

static ObjMesh loadObjMesh(const char *path)
{
    ObjMesh mesh = {0};
    FILE *f;
    char line[512];
    int vCount = 0, vtCount = 0, vnCount = 0, faceCount = 0;
    float *rawV, *rawVt, *rawVn;
    float *outV, *outUV, *outN;
    int vi, vti, vni, outi, maxOutVerts;

    {
        static const char *prefixes[] = {"", "../", "../../", "../../../"};
        char candidate[768];
        int i;

        f = NULL;
        for (i = 0; i < (int)(sizeof(prefixes) / sizeof(prefixes[0])); i++)
        {
            snprintf(candidate, sizeof(candidate), "%s%s", prefixes[i], path);
            f = fopen(candidate, "r");
            if (f)
            {
                if (i != 0)
                {
                    fprintf(stderr, "Info: gate mesh loaded via fallback path: %s\n", candidate);
                }
                break;
            }
        }
    }
    if (!f)
    {
        fprintf(stderr, "loadObjMesh: cannot open %s\n", path);
        return mesh;
    }

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "v ", 2) == 0) vCount++;
        else if (strncmp(line, "vt ", 3) == 0) vtCount++;
        else if (strncmp(line, "vn ", 3) == 0) vnCount++;
        else if (strncmp(line, "f ", 2) == 0) faceCount++;
    }
    rewind(f);

    if (vCount == 0 || faceCount == 0)
    {
        fclose(f);
        return mesh;
    }

    rawV = (float*)malloc(vCount * 3 * sizeof(float));
    rawVt = (float*)malloc(vtCount * 2 * sizeof(float));
    rawVn = (float*)malloc(vnCount * 3 * sizeof(float));
    maxOutVerts = faceCount * 6;
    outV = (float*)malloc(maxOutVerts * 3 * sizeof(float));
    outUV = (float*)malloc(maxOutVerts * 2 * sizeof(float));
    outN = (float*)malloc(maxOutVerts * 3 * sizeof(float));

    if (!rawV || !rawVt || !rawVn || !outV || !outUV || !outN)
    {
        free(rawV);
        free(rawVt);
        free(rawVn);
        free(outV);
        free(outUV);
        free(outN);
        fclose(f);
        return mesh;
    }

    vi = 0;
    vti = 0;
    vni = 0;
    outi = 0;

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "v ", 2) == 0)
        {
            sscanf(line + 2, "%f %f %f", &rawV[vi * 3], &rawV[vi * 3 + 1], &rawV[vi * 3 + 2]);
            vi++;
        }
        else if (strncmp(line, "vt ", 3) == 0)
        {
            sscanf(line + 3, "%f %f", &rawVt[vti * 2], &rawVt[vti * 2 + 1]);
            vti++;
        }
        else if (strncmp(line, "vn ", 3) == 0)
        {
            sscanf(line + 3, "%f %f %f", &rawVn[vni * 3], &rawVn[vni * 3 + 1], &rawVn[vni * 3 + 2]);
            vni++;
        }
        else if (strncmp(line, "f ", 2) == 0)
        {
            int fv[8];
            int ft[8];
            int fn[8];
            int nfv;
            int ti;
            int j;
            char *p;

            nfv = 0;
            p = line + 2;
            while (*p && nfv < 8)
            {
                int v = 0;
                int t = 0;
                int n = 0;

                while (*p == ' ' || *p == '\t') p++;
                if (!*p || *p == '\n' || *p == '\r') break;
                while (*p >= '0' && *p <= '9')
                {
                    v = v * 10 + (*p - '0');
                    p++;
                }
                if (*p == '/')
                {
                    p++;
                    if (*p != '/' && *p >= '0' && *p <= '9')
                    {
                        while (*p >= '0' && *p <= '9')
                        {
                            t = t * 10 + (*p - '0');
                            p++;
                        }
                    }
                    if (*p == '/')
                    {
                        p++;
                        while (*p >= '0' && *p <= '9')
                        {
                            n = n * 10 + (*p - '0');
                            p++;
                        }
                    }
                }
                if (v > 0)
                {
                    fv[nfv] = v;
                    ft[nfv] = t;
                    fn[nfv] = n;
                    nfv++;
                }
            }
            for (ti = 1; ti < nfv - 1 && outi + 3 <= maxOutVerts; ti++)
            {
                int idxs[3];

                idxs[0] = 0;
                idxs[1] = ti;
                idxs[2] = ti + 1;
                for (j = 0; j < 3; j++)
                {
                    int v1 = fv[idxs[j]] - 1;
                    int t1 = ft[idxs[j]] - 1;
                    int n1 = fn[idxs[j]] - 1;

                    outV[outi * 3] = rawV[v1 * 3];
                    outV[outi * 3 + 1] = rawV[v1 * 3 + 1];
                    outV[outi * 3 + 2] = rawV[v1 * 3 + 2];
                    outUV[outi * 2] = (t1 >= 0 && t1 < vtCount) ? rawVt[t1 * 2] : 0.0f;
                    outUV[outi * 2 + 1] = (t1 >= 0 && t1 < vtCount) ? rawVt[t1 * 2 + 1] : 0.0f;
                    outN[outi * 3] = (n1 >= 0 && n1 < vnCount) ? rawVn[n1 * 3] : 0.0f;
                    outN[outi * 3 + 1] = (n1 >= 0 && n1 < vnCount) ? rawVn[n1 * 3 + 1] : 1.0f;
                    outN[outi * 3 + 2] = (n1 >= 0 && n1 < vnCount) ? rawVn[n1 * 3 + 2] : 0.0f;
                    outi++;
                }
            }
        }
    }
    fclose(f);
    free(rawV);
    free(rawVt);
    free(rawVn);

    mesh.verts = outV;
    mesh.uvs = outUV;
    mesh.normals = outN;
    mesh.count = outi;
    if (outi > 0)
    {
        int k;

        mesh.minX = outV[0];
        mesh.minY = outV[1];
        mesh.minZ = outV[2];
        mesh.maxX = outV[0];
        mesh.maxY = outV[1];
        mesh.maxZ = outV[2];

        for (k = 1; k < outi; k++)
        {
            float x = outV[k * 3];
            float y = outV[k * 3 + 1];
            float z = outV[k * 3 + 2];

            if (x < mesh.minX) mesh.minX = x;
            if (y < mesh.minY) mesh.minY = y;
            if (z < mesh.minZ) mesh.minZ = z;
            if (x > mesh.maxX) mesh.maxX = x;
            if (y > mesh.maxY) mesh.maxY = y;
            if (z > mesh.maxZ) mesh.maxZ = z;
        }
    }
    printf("loadObjMesh: %d triangles loaded from %s\n", outi / 3, path);
    return mesh;
}

void loadGateMesh(const char *path)
{
    gateMesh = loadObjMesh(path);
}

void loadSkeletonMesh(const char *path)
{
    skeletonMesh = loadObjMesh(path);
}

int hasSkeletonMesh(void)
{
    return skeletonMesh.count > 0;
}

static void drawObjMesh(const ObjMesh *mesh)
{
    int i;

    if (!mesh || mesh->count == 0)
    {
        return;
    }

    glBegin(GL_TRIANGLES);
    for (i = 0; i < mesh->count; i++)
    {
        glNormal3f(mesh->normals[i * 3], mesh->normals[i * 3 + 1], mesh->normals[i * 3 + 2]);
        glTexCoord2f(mesh->uvs[i * 2], mesh->uvs[i * 2 + 1]);
        glVertex3f(mesh->verts[i * 3], mesh->verts[i * 3 + 1], mesh->verts[i * 3 + 2]);
    }
    glEnd();
}

void drawStarterGate(void)
{
    const float objXMin = -0.0363f;
    const float objYMin = -0.0079f;
    const float objZMin = -0.0239f;
    const float objZMax = 0.0615f;
    const float objHeight = 2.9208f;
    GLfloat matWhite[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float scale;
    float zCenter;

    if (!starterGate.active)
    {
        return;
    }

    scale = WALL_HEIGHT / objHeight;
    zCenter = (objZMin + objZMax) * 0.5f;

    glPushMatrix();
    glTranslatef(starterGate.hingeX, 0.0f, starterGate.hingeZ);
    glRotatef(starterGate.yawDeg, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);
    glRotatef(-starterGate.currentAngleDeg, 0.0f, 1.0f, 0.0f);
    glTranslatef(-objXMin, -objYMin, -zCenter);

    if (gateTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gateTexture);
        setWorldShaderUseTexture(1);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        setWorldShaderUseTexture(0);
        glColor3f(0.70f, 0.70f, 0.74f);
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matWhite);

    drawObjMesh(&gateMesh);

    if (gateTexture != 0)
    {
        glDisable(GL_TEXTURE_2D);
    }
    setWorldShaderUseTexture(0);
    glPopMatrix();
}

void drawSkeletonMeshAt(float worldX, float worldZ, float faceX, float faceZ, float targetHeight)
{
    float height;
    float centerX;
    float centerZ;
    float yaw;
    float scale;
    GLfloat matGray[] = {0.36f, 0.36f, 0.36f, 1.0f};

    if (skeletonMesh.count <= 0)
    {
        return;
    }

    height = skeletonMesh.maxY - skeletonMesh.minY;
    if (height <= 0.0001f)
    {
        return;
    }

    centerX = (skeletonMesh.minX + skeletonMesh.maxX) * 0.5f;
    centerZ = (skeletonMesh.minZ + skeletonMesh.maxZ) * 0.5f;
    yaw = atan2f(faceZ, faceX) * (180.0f / (float)M_PI);
    scale = targetHeight / height;

    glPushMatrix();
    glTranslatef(worldX, 0.0f, worldZ);
    glRotatef(-yaw + 90.0f, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);
    glTranslatef(-centerX, -skeletonMesh.minY, -centerZ);

    glDisable(GL_TEXTURE_2D);
    setWorldShaderUseTexture(0);
    glColor3f(0.36f, 0.36f, 0.36f);

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matGray);
    drawObjMesh(&skeletonMesh);

    setWorldShaderUseTexture(0);
    glPopMatrix();
}

void loadZombieMesh(const char *path)
{
    zombieMesh = loadObjMesh(path);
}

int hasZombieMesh(void)
{
    return zombieMesh.count > 0;
}

void drawZombieAt(float worldX, float worldZ, float faceX, float faceZ, float targetHeight)
{
    float height;
    float centerX;
    float centerZ;
    float yaw;
    float scale;
    GLfloat matWhite[] = {1.0f, 1.0f, 1.0f, 1.0f};

    if (zombieMesh.count <= 0)
    {
        return;
    }

    height = zombieMesh.maxY - zombieMesh.minY;
    if (height <= 0.0001f)
    {
        return;
    }

    centerX = (zombieMesh.minX + zombieMesh.maxX) * 0.5f;
    centerZ = (zombieMesh.minZ + zombieMesh.maxZ) * 0.5f;
    yaw = atan2f(faceZ, faceX) * (180.0f / (float)M_PI);
    scale = targetHeight / height;

    glPushMatrix();
    glTranslatef(worldX, 0.0f, worldZ);
    glRotatef(-yaw + 90.0f, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);
    glTranslatef(-centerX, -zombieMesh.minY, -centerZ);

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matWhite);
    drawObjMesh(&zombieMesh);

    glPopMatrix();
}