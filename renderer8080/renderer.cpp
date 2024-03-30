#include "renderer.h"
#include <iostream>

using namespace std;

Renderer8080::Renderer8080() {}

/* Render pixels from VRam, scanning from the bottom left to the top right. Each segment of y comprises 8 bits to examine. */
void Renderer8080::RenderPixels(CPU::State8080* state)
{
	SDL_RenderClear(sdlRenderer);
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 0);
	int vRamAddress = 0x2400;
	// We render from the bottom of column one to the top of column one, then proceed to the next column
	for (int xPixel = 0; xPixel < XPixelCount; ++xPixel)
	{
		for (int yPixel = 0; yPixel < YPixelCount; yPixel += 8) // increment one byte per pass
		{
			uint8_t videoByte = state->mem[vRamAddress];
			vRamAddress += 1;

			// draw the pixels
			for (int bitIndex = 0; bitIndex < 8; ++bitIndex)
			{
				// get the right most bit by itself, if 1 draw pixel is true else false
				bool drawPixel = videoByte & 0x1;
				int yPos = (yPixel + 8) + bitIndex;
				int xPos = xPixel;
				if (drawPixel)
				{
					// draw gameobject color pixel at xPos and yPos
					SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
				}
				else
				{
					// draw background color pixel at xPos and yPos
					SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
				}
				SDL_RenderDrawPoint(sdlRenderer, xPos, YPixelCount - yPos);
				// so shift right to check the next bit
				videoByte = videoByte >> 1;
			}
		}
	}
	SDL_RenderPresent(sdlRenderer);
}

void Renderer8080::init()
{
	int scaledWindowWidth = XPixelCount * WindowScaleFactor;
	int scaledWindowHeight = YPixelCount * WindowScaleFactor;
	window = SDL_CreateWindow("INCEPTION",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		scaledWindowWidth, scaledWindowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN); // todo - set SDL_WINDOW_RESIZABLE flag and make scaling
	sdlRenderer = SDL_CreateRenderer(window, 0, 0);
	SDL_RenderSetLogicalSize(sdlRenderer, XPixelCount, YPixelCount);
	SDL_SetWindowTitle(window, "INTEL 8080 EMULATOR");
}

void Renderer8080::destory()
{
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(window);
}
