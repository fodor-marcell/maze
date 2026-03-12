#include <math.h>
#include <stdio.h>

#include "glshader.h"

typedef char GLchar;

typedef GLuint (*GLCreateShaderProc)(GLenum type);
typedef void (*GLShaderSourceProc)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
typedef void (*GLCompileShaderProc)(GLuint shader);
typedef void (*GLGetShaderivProc)(GLuint shader, GLenum pname, GLint* params);
typedef void (*GLGetShaderInfoLogProc)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef GLuint (*GLCreateProgramProc)(void);
typedef void (*GLAttachShaderProc)(GLuint program, GLuint shader);
typedef void (*GLLinkProgramProc)(GLuint program);
typedef void (*GLGetProgramivProc)(GLuint program, GLenum pname, GLint* params);
typedef void (*GLGetProgramInfoLogProc)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef void (*GLUseProgramProc)(GLuint program);
typedef void (*GLDeleteShaderProc)(GLuint shader);
typedef void (*GLDeleteProgramProc)(GLuint program);
typedef GLint (*GLGetUniformLocationProc)(GLuint program, const GLchar* name);
typedef void (*GLUniform1iProc)(GLint location, GLint v0);
typedef void (*GLUniform1fProc)(GLint location, GLfloat v0);
typedef void (*GLUniform3fProc)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);

static GLCreateShaderProc pglCreateShader = 0;
static GLShaderSourceProc pglShaderSource = 0;
static GLCompileShaderProc pglCompileShader = 0;
static GLGetShaderivProc pglGetShaderiv = 0;
static GLGetShaderInfoLogProc pglGetShaderInfoLog = 0;
static GLCreateProgramProc pglCreateProgram = 0;
static GLAttachShaderProc pglAttachShader = 0;
static GLLinkProgramProc pglLinkProgram = 0;
static GLGetProgramivProc pglGetProgramiv = 0;
static GLGetProgramInfoLogProc pglGetProgramInfoLog = 0;
static GLUseProgramProc pglUseProgram = 0;
static GLDeleteShaderProc pglDeleteShader = 0;
static GLDeleteProgramProc pglDeleteProgram = 0;
static GLGetUniformLocationProc pglGetUniformLocation = 0;
static GLUniform1iProc pglUniform1i = 0;
static GLUniform1fProc pglUniform1f = 0;
static GLUniform3fProc pglUniform3f = 0;

static GLuint worldShaderProgram = 0;
static GLint uUseTextureLoc = -1;
static GLint uTextureLoc = -1;
static GLint uLightPosEyeLoc = -1;
static GLint uLightDirEyeLoc = -1;
static GLint uLightColorLoc = -1;
static GLint uAmbientLoc = -1;
static GLint uCutoffCosLoc = -1;
static GLint uExponentLoc = -1;
static GLint uAttenConstLoc = -1;
static GLint uAttenLinearLoc = -1;
static GLint uAttenQuadLoc = -1;

static void* getGLProcAddress(const char* name)
{
    void* proc = (void*)wglGetProcAddress(name);

    if (proc == 0 || proc == (void*)0x1 || proc == (void*)0x2 || proc == (void*)0x3 || proc == (void*)-1)
    {
        static HMODULE glModule = 0;
        if (glModule == 0)
        {
            glModule = LoadLibraryA("opengl32.dll");
        }
        if (glModule != 0)
        {
            proc = (void*)GetProcAddress(glModule, name);
        }
    }

    return proc;
}

static int loadShaderFunctions(void)
{
    pglCreateShader = (GLCreateShaderProc)getGLProcAddress("glCreateShader");
    pglShaderSource = (GLShaderSourceProc)getGLProcAddress("glShaderSource");
    pglCompileShader = (GLCompileShaderProc)getGLProcAddress("glCompileShader");
    pglGetShaderiv = (GLGetShaderivProc)getGLProcAddress("glGetShaderiv");
    pglGetShaderInfoLog = (GLGetShaderInfoLogProc)getGLProcAddress("glGetShaderInfoLog");
    pglCreateProgram = (GLCreateProgramProc)getGLProcAddress("glCreateProgram");
    pglAttachShader = (GLAttachShaderProc)getGLProcAddress("glAttachShader");
    pglLinkProgram = (GLLinkProgramProc)getGLProcAddress("glLinkProgram");
    pglGetProgramiv = (GLGetProgramivProc)getGLProcAddress("glGetProgramiv");
    pglGetProgramInfoLog = (GLGetProgramInfoLogProc)getGLProcAddress("glGetProgramInfoLog");
    pglUseProgram = (GLUseProgramProc)getGLProcAddress("glUseProgram");
    pglDeleteShader = (GLDeleteShaderProc)getGLProcAddress("glDeleteShader");
    pglDeleteProgram = (GLDeleteProgramProc)getGLProcAddress("glDeleteProgram");
    pglGetUniformLocation = (GLGetUniformLocationProc)getGLProcAddress("glGetUniformLocation");
    pglUniform1i = (GLUniform1iProc)getGLProcAddress("glUniform1i");
    pglUniform1f = (GLUniform1fProc)getGLProcAddress("glUniform1f");
    pglUniform3f = (GLUniform3fProc)getGLProcAddress("glUniform3f");

    return pglCreateShader && pglShaderSource && pglCompileShader && pglGetShaderiv
        && pglGetShaderInfoLog && pglCreateProgram && pglAttachShader && pglLinkProgram
        && pglGetProgramiv && pglGetProgramInfoLog && pglUseProgram && pglDeleteShader
        && pglDeleteProgram && pglGetUniformLocation && pglUniform1i && pglUniform1f
        && pglUniform3f;
}

static GLuint compileShader(GLenum shaderType, const char* source)
{
    GLuint shader = pglCreateShader(shaderType);
    GLint status = 0;

    pglShaderSource(shader, 1, (const GLchar**)&source, 0);
    pglCompileShader(shader);
    pglGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        GLchar log[2048];
        GLsizei logLen = 0;
        pglGetShaderInfoLog(shader, (GLsizei)sizeof(log) - 1, &logLen, log);
        log[logLen] = '\0';
        fprintf(stderr, "Shader compile failed: %s\n", log);
        pglDeleteShader(shader);
        return 0;
    }

    return shader;
}

static void multMat4Vec4(const GLfloat* m, const GLfloat* v, GLfloat* out)
{
    out[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
    out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
    out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
    out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}

static void multMat3Vec3(const GLfloat* m, const GLfloat* v, GLfloat* out)
{
    out[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2];
    out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2];
    out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2];
}

int initWorldShaderProgram(void)
{
    static const char* vertexShaderSrc =
        "#version 120\n"
        "varying vec3 vPosEye;\n"
        "varying vec3 vNormalEye;\n"
        "varying vec2 vTexCoord;\n"
        "void main()\n"
        "{\n"
        "    vec4 posEye = gl_ModelViewMatrix * gl_Vertex;\n"
        "    vPosEye = posEye.xyz;\n"
        "    vNormalEye = normalize(gl_NormalMatrix * gl_Normal);\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    gl_FrontColor = gl_Color;\n"
        "}\n";

    static const char* fragmentShaderSrc =
        "#version 120\n"
        "uniform sampler2D uTexture;\n"
        "uniform int uUseTexture;\n"
        "uniform vec3 uLightPosEye;\n"
        "uniform vec3 uLightDirEye;\n"
        "uniform vec3 uLightColor;\n"
        "uniform vec3 uAmbient;\n"
        "uniform float uCutoffCos;\n"
        "uniform float uExponent;\n"
        "uniform float uAttenConstant;\n"
        "uniform float uAttenLinear;\n"
        "uniform float uAttenQuad;\n"
        "varying vec3 vPosEye;\n"
        "varying vec3 vNormalEye;\n"
        "varying vec2 vTexCoord;\n"
        "void main()\n"
        "{\n"
        "    vec4 baseColor = (uUseTexture != 0) ? texture2D(uTexture, vTexCoord) : gl_Color;\n"
        "    vec3 N = normalize(vNormalEye);\n"
        "    vec3 L = normalize(uLightPosEye - vPosEye);\n"
        "    vec3 toFrag = normalize(vPosEye - uLightPosEye);\n"
        "    float dist = length(uLightPosEye - vPosEye);\n"
        "    float spotCos = dot(normalize(uLightDirEye), toFrag);\n"
        "    float spotFactor = (spotCos >= uCutoffCos) ? pow(max(spotCos, 0.0), uExponent) : 0.0;\n"
        "    float lambert = max(dot(N, L), 0.0);\n"
        "    float atten = 1.0 / (uAttenConstant + uAttenLinear * dist + uAttenQuad * dist * dist);\n"
        "    float lit = lambert * spotFactor * atten;\n"
        "    vec3 finalRgb = baseColor.rgb * (uAmbient + uLightColor * lit);\n"
        "    gl_FragColor = vec4(finalRgb, baseColor.a);\n"
        "}\n";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLint linkStatus = 0;

    if (!loadShaderFunctions())
    {
        fprintf(stderr, "GLSL unavailable, falling back to fixed pipeline lighting.\n");
        return 0;
    }

    vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    if (vertexShader == 0 || fragmentShader == 0)
    {
        if (vertexShader != 0)
        {
            pglDeleteShader(vertexShader);
        }
        if (fragmentShader != 0)
        {
            pglDeleteShader(fragmentShader);
        }
        return 0;
    }

    worldShaderProgram = pglCreateProgram();
    pglAttachShader(worldShaderProgram, vertexShader);
    pglAttachShader(worldShaderProgram, fragmentShader);
    pglLinkProgram(worldShaderProgram);
    pglGetProgramiv(worldShaderProgram, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus)
    {
        GLchar log[2048];
        GLsizei logLen = 0;
        pglGetProgramInfoLog(worldShaderProgram, (GLsizei)sizeof(log) - 1, &logLen, log);
        log[logLen] = '\0';
        fprintf(stderr, "Program link failed: %s\n", log);
        pglDeleteShader(vertexShader);
        pglDeleteShader(fragmentShader);
        pglDeleteProgram(worldShaderProgram);
        worldShaderProgram = 0;
        return 0;
    }

    pglDeleteShader(vertexShader);
    pglDeleteShader(fragmentShader);

    uUseTextureLoc = pglGetUniformLocation(worldShaderProgram, "uUseTexture");
    uTextureLoc = pglGetUniformLocation(worldShaderProgram, "uTexture");
    uLightPosEyeLoc = pglGetUniformLocation(worldShaderProgram, "uLightPosEye");
    uLightDirEyeLoc = pglGetUniformLocation(worldShaderProgram, "uLightDirEye");
    uLightColorLoc = pglGetUniformLocation(worldShaderProgram, "uLightColor");
    uAmbientLoc = pglGetUniformLocation(worldShaderProgram, "uAmbient");
    uCutoffCosLoc = pglGetUniformLocation(worldShaderProgram, "uCutoffCos");
    uExponentLoc = pglGetUniformLocation(worldShaderProgram, "uExponent");
    uAttenConstLoc = pglGetUniformLocation(worldShaderProgram, "uAttenConstant");
    uAttenLinearLoc = pglGetUniformLocation(worldShaderProgram, "uAttenLinear");
    uAttenQuadLoc = pglGetUniformLocation(worldShaderProgram, "uAttenQuad");

    pglUseProgram(worldShaderProgram);
    pglUniform1i(uTextureLoc, 0);
    pglUniform1i(uUseTextureLoc, 0);
    pglUseProgram(0);

    return 1;
}

void beginWorldShader(const GLfloat lightPosWorld[4], const GLfloat lightDirWorld[3],
    const GLfloat diffuse[4], const GLfloat ambient[4], float cutoffDeg, float exponent,
    float attenConstant, float attenLinear, float attenQuad)
{
    GLfloat mv[16];
    GLfloat lightPosEye4[4];
    GLfloat lightDirEye[3];
    float dirLen;

    if (worldShaderProgram == 0)
    {
        return;
    }

    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    multMat4Vec4(mv, lightPosWorld, lightPosEye4);
    multMat3Vec3(mv, lightDirWorld, lightDirEye);

    dirLen = sqrtf(lightDirEye[0] * lightDirEye[0] + lightDirEye[1] * lightDirEye[1] + lightDirEye[2] * lightDirEye[2]);
    if (dirLen > 0.0001f)
    {
        lightDirEye[0] /= dirLen;
        lightDirEye[1] /= dirLen;
        lightDirEye[2] /= dirLen;
    }

    pglUseProgram(worldShaderProgram);
    pglUniform3f(uLightPosEyeLoc, lightPosEye4[0], lightPosEye4[1], lightPosEye4[2]);
    pglUniform3f(uLightDirEyeLoc, lightDirEye[0], lightDirEye[1], lightDirEye[2]);
    pglUniform3f(uLightColorLoc, diffuse[0], diffuse[1], diffuse[2]);
    pglUniform3f(uAmbientLoc, ambient[0], ambient[1], ambient[2]);
    pglUniform1f(uCutoffCosLoc, cosf(cutoffDeg * (3.14159265358979323846f / 180.0f)));
    pglUniform1f(uExponentLoc, exponent);
    pglUniform1f(uAttenConstLoc, attenConstant);
    pglUniform1f(uAttenLinearLoc, attenLinear);
    pglUniform1f(uAttenQuadLoc, attenQuad);
    pglUniform1i(uUseTextureLoc, 0);
}

void setWorldShaderUseTexture(int enabled)
{
    if (worldShaderProgram == 0)
    {
        return;
    }

    pglUniform1i(uUseTextureLoc, enabled ? 1 : 0);
}

void endWorldShader(void)
{
    if (worldShaderProgram != 0)
    {
        pglUseProgram(0);
    }
}
