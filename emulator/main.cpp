#include <fstream>
#include <chrono>
#include <filesystem>
#include <thread>
#include <atomic>
#include "emulator_shell.h"
#include "../inputoutput/inputHandler.h"
#include "../renderer8080/renderer.h"

using namespace std;
using namespace std::chrono;
namespace FileSystem = std::filesystem;
std::atomic<bool> quit{false};


void ReadFileIntoMemoryAt(CPU::State8080 *state, const char *filename, uint32_t offset)
{
    FILE *f;
    std::filesystem::path romPath = filename;
    string filepath = FileSystem::absolute(romPath).string();
    const char *rawFilePath = filepath.c_str();
    const char *mode = "rb";
#ifdef _WIN32
    errno_t err = fopen_s(&f, rawFilePath, mode);
#else
    f = fopen(rawFilePath, mode);
#endif
    if (f == NULL)
    {
        printf("error: Couldn't open %s\n", filename);
        exit(1);
    }
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    uint8_t *buffer = &state->mem[offset];
    fread(buffer, fsize, 1, f);
    fclose(f);
}

CPU::State8080 *Init8080(void)
{
    // allocate initialized data for cpu state
    CPU::State8080 *state = (CPU::State8080 *)calloc(1, sizeof(CPU::State8080));
    // point state->mem toward 16k of memory
    state->mem = (uint8_t *)malloc(0x10000); // 16K
    return state;
}

void RenderGraphics(CPU::State8080 *state, milliseconds startingTime, milliseconds currentTime, Renderer8080 *vRender)
{
    const int frameTime = 100;
    while (!quit)
    {
        currentTime = duration_cast<milliseconds>(chrono::system_clock::now().time_since_epoch());
        if (currentTime.count() - startingTime.count() > frameTime)
        {
            vRender->RenderPixels(state);
            startingTime = duration_cast<milliseconds>(chrono::system_clock::now().time_since_epoch());
        }
    }
}

int main(int argc, char **argv)
{
    int done = 0;
    CPU::State8080 *state = Init8080();
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Event event;
    PortLoader8080 portLoader;
    // store the beginning of the memory for state
    uint8_t *mem_start = state->mem;
    // load the rom files into memory
    ReadFileIntoMemoryAt(state, "ROM/invaders.h", 0);
    ReadFileIntoMemoryAt(state, "ROM/invaders.g", 0x800);
    ReadFileIntoMemoryAt(state, "ROM/invaders.f", 0x1000);
    ReadFileIntoMemoryAt(state, "ROM/invaders.e", 0x1800);
    // we need an instance of CPU to call the Emulator8080 codes
    CPU cpu_instance;
    // Run rendering on RenderThread
    milliseconds startingTime, currentTime = duration_cast<milliseconds>(chrono::system_clock::now().time_since_epoch());
    Renderer8080 *vRender = new Renderer8080();
    vRender->init();
    thread RenderThread(RenderGraphics, state, startingTime, currentTime, vRender);
    // Run CPU on Main Thread
    CPU::AudioBootup();
    int i;
    while (done == 0)
    {
        i = 0;
        while (i < 10000 && !state->halted)
        {
            i++;
            done = cpu_instance.Emulate8080Codes(state);
        }
        cpu_instance.PerformInterrupt(state);
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                done = 1;
                quit = true;
            }
            portLoader.PortLoader(state, event);
        }
        this_thread::sleep_for(milliseconds(15));
    }
    RenderThread.join();
    CPU::AudioTearDown();
    vRender->destory();
    SDL_Quit();
    free(mem_start);
    return 0;
}

// state->halted = false;
// auto interval = std::chrono::milliseconds(1000 / 60);
// auto start_time = std::chrono::steady_clock::now();
// // Run CPU on Main Thread
// while (done == 0)
// {
//     if (!state->halted)
//     {
//         done = cpu_instance.Emulate8080Codes(state);
//     }
//     else
//     {

//         state->halted = false;
//         cpu_instance.PerformInterrupt(state);
//         start_time = std::chrono::steady_clock::now();
//     }
//     auto end_time = std::chrono::steady_clock::now();
//     auto elapsed_time = end_time - start_time;
//     if (elapsed_time > interval)
//     {
//         state->halted = false;
//     }
//     this_thread::sleep_for(milliseconds(10));
// free(mem_start);
// return 0;
// }