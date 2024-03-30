#pragma once

#include <cstdint>
#include <SDL.h>
#include "../emulator/emulator_shell.h"

class Renderer8080 {

public:
    Renderer8080();

    const int XPixelCount = 224;
    const int YPixelCount = 256;
    const int WindowScaleFactor = 3;

    void RenderPixels(CPU::State8080* state);

    void init();

    void destory();

    SDL_Event event;
    SDL_Renderer* sdlRenderer;
    SDL_Window* window;

};