#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL_opengl.h>

#include "model.h"

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