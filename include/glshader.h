#ifndef GLSHADER_H
#define GLSHADER_H

#include <windows.h>
#include <GL/glut.h>

#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif

#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif

int initWorldShaderProgram(void);
void beginWorldShader(const GLfloat lightPosWorld[4], const GLfloat lightDirWorld[3],
    const GLfloat diffuse[4], const GLfloat ambient[4], float cutoffDeg, float exponent,
    float attenConstant, float attenLinear, float attenQuad);
void setWorldShaderUseTexture(int enabled);
void endWorldShader(void);

#endif
