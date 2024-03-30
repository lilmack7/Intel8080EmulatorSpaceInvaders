#include "inputHandler.h"
#include <iostream>

PortLoader8080::PortLoader8080(){}

void PortLoader8080::PortLoader(CPU::State8080 *state, SDL_Event event)
{
    if (event.type == SDL_KEYDOWN)
    {
        // std::cout << "Key Down: " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
        switch (event.key.keysym.sym)
        {
        case SDLK_c: // coin
            state->port1 |= 1;
            break;
        case SDLK_g: // p2 start
            state->port1 |= 1 << 1;
            break;
        case SDLK_h: // p1 start
            state->port1 |= 1 << 2;
            break;
        case SDLK_i: // p1 shoot
            state->port1 |= 1 << 4;
            break;
        case SDLK_j: // p1 left
            state->port1 |= 1 << 5;
            state->port1 &= ~(1 << 6);
            break;
        case SDLK_l: // p1 right
            state->port1 |= 1 << 6;
            state->port1 &= ~(1 << 5);
            break;
        case SDLK_e: // p2 shoot
            state->port2 |= 1 << 4;
            break;
        case SDLK_s: // p2 left
            state->port2 |= 1 << 5;
            state->port2 &= ~(1 << 6);
            break;
        case SDLK_f: // p2 right
            state->port2 |= 1 << 6;
            state->port2 &= ~(1 << 5);
            break;
        }
    }
    else if (event.type == SDL_KEYUP)
    {
        // std::cout << "Key Up: " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
        switch (event.key.keysym.sym)
        {
        case SDLK_c: // coin
            state->port1 &= ~1;
            break;
        case SDLK_g: // p2 start
            state->port1 &= ~(1 << 1);
            break;
        case SDLK_h: // p1 start
            state->port1 &= ~(1 << 2);
            break;
        case SDLK_i: // p1 shoot
            state->port1 &= ~(1 << 4);
            break;
        case SDLK_j: // p1 left
            state->port1 &= ~(1 << 5);
            break;
        case SDLK_l: // p1 right
            state->port1 &= ~(1 << 6);
            break;
        case SDLK_e: // p2 shoot
            state->port2 &= ~(1 << 4);
            break;
        case SDLK_s: // p2 left
            state->port2 &= ~(1 << 5);
            break;
        case SDLK_f: // p2 right
            state->port2 &= ~(1 << 6);
            break;
        }
    }
}
