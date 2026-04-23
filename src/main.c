#include <windows.h>

#include <SDL.h>
#include <SDL_image.h>

#include "game.h"
#include "model.h"
#include "render.h"

static void cleanup(void)
{
    shutdownBitmapFont();
    freeModel(&gTableModel);

    if (tableTexture != 0)
    {
        glDeleteTextures(1, &tableTexture);
    }

    if (skyTexture != 0)
    {
        glDeleteTextures(1, &skyTexture);
    }

    if (floorTexture != 0)
    {
        glDeleteTextures(1, &floorTexture);
    }

    if (wallTexture != 0)
    {
        glDeleteTextures(1, &wallTexture);
    }

    if (gGLContext)
    {
        SDL_GL_DeleteContext(gGLContext);
        gGLContext = NULL;
    }

    if (gWindow)
    {
        SDL_DestroyWindow(gWindow);
        gWindow = NULL;
    }

    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char** argv)
{
    SDL_Event e;
    int imgFlags;

    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return 1;
    }

    imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags)
    {
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    gWindow = SDL_CreateWindow(
        "Maze - Labyrinthine Style Flashlight",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1000,
        700,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (!gWindow)
    {
        cleanup();
        return 1;
    }

    gGLContext = SDL_GL_CreateContext(gWindow);
    if (!gGLContext)
    {
        cleanup();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);

    SDL_GetWindowSize(gWindow, &windowWidth, &windowHeight);
    reshape(windowWidth, windowHeight);

    init();

    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    gRunning = 1;
    while (gRunning)
    {
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    gRunning = 0;
                    break;
                case SDL_KEYDOWN:
                    keyboard(e.key.keysym.sym, 0, 0);
                    break;
                case SDL_KEYUP:
                    keyboardUp(e.key.keysym.sym, 0, 0);
                    break;
                case SDL_MOUSEMOTION:
                    mouseMotion(e.motion.xrel, e.motion.yrel);
                    break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        reshape(e.window.data1, e.window.data2);
                    }
                    break;
                default:
                    break;
            }
        }

        update();
        display();
        SDL_GL_SwapWindow(gWindow);
    }

    cleanup();
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    return main(0, NULL);
}
#endif