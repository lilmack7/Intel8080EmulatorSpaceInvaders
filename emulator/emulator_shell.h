#pragma once

#include <cstdint>
#include <SDL_mixer.h>

class CPU {

public:
    CPU();

// Defines FlagCodes structure for tracking/adjusting flags in F register
// pad variable for bits 5, 3, and 1 which retain set value (not used)
// bit 5 always 0, bit 3 always 0, bit 1 always 1
    typedef struct FlagCodes {
        uint8_t s: 1;
        uint8_t z: 1;
        uint8_t ac: 1;
        uint8_t p: 1;
        uint8_t cy: 1;
        uint8_t pad: 3;
    } FlagCodes;

// Defines structure for tracking each of the registers found in Intel's 8080
// Also includes instance of FlagCodes struct to serve as our f register for flags
    typedef struct State8080 {
        uint8_t a;
        uint8_t b;
        uint8_t c;
        uint8_t d;
        uint8_t e;
        uint8_t h;
        uint8_t l;
        uint16_t sp;
        uint16_t pc;
        uint8_t *mem;
        uint8_t int_enable;
        uint8_t port1;
        uint8_t port2;
        uint8_t out_port3;
        uint8_t out_port5;
        uint8_t out_port3_prev;
        uint8_t out_port5_prev;
        bool halted;
        FlagCodes f;
    } State8080;

    void UnimplementedInstruction(State8080 *state);

    int Emulate8080Codes(State8080 *state);

    FlagCodes SetFlags(uint16_t result);

    bool Parity(uint16_t number);

    uint8_t FlagCalc(FlagCodes flagState);
  
    bool IsAuxFlagSet(uint16_t number);

    void PerformInterrupt(State8080* state);

    static void AudioBootup();

    static void AudioTearDown();

private:
    int interruptNumber = 1;

    uint8_t     shift0          = 0;
    uint8_t     shift1          = 0;
    uint8_t     shift_offset    = 0;

    void HandleInput(State8080* state, uint8_t port);
    void HandleOutput(uint8_t port, uint8_t value, State8080 *state);
    void PlayAudio(State8080 *state);


  
};
