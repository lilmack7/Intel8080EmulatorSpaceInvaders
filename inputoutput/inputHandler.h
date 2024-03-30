#pragma once

#include <cstdint>
#include "../emulator/emulator_shell.h"
#include <SDL.h>

class PortLoader8080 {

public:
    PortLoader8080();
    void PortLoader(CPU::State8080 *state, SDL_Event event);
private:
    SDL_Event event;
};