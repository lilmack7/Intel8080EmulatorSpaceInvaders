#include <iostream>
#include "emulator_shell.h"
#include "emulator_shell.h"
#include "disassembler.h"

using namespace std;

CPU::CPU() {}

// Return true if even parity and false if odd parity
bool CPU::Parity(uint16_t number)
{
    bool parity = true;
    while (number)
    {
        parity = !parity;
        number = number & (number - 1);
    }
    return parity;
}

// Placeholder function for currently unimplemented instructions
// Lets us track where we are with getting instruction function up and working
void CPU::UnimplementedInstruction(State8080 *state)
{
    // Undoes incrementing of pc register
    (state->pc)--;
    cout << "Error: Currently unimplemented instruction" << endl;
    exit(1);
}

CPU::FlagCodes CPU::SetFlags(uint16_t result)
{
    FlagCodes ResultCodes;
    // check if zero flag should be set
    ResultCodes.z = (result == 0);

    // Sign flag - most significant bit is set
    ResultCodes.s = 0x80 == (result & 0x80);

    // Parity flag, check if the last bit is set
    ResultCodes.p = Parity(result);

    // Carry flag will be true if result > 255 (1111 1111)
    ResultCodes.cy = (result > 0xff);

    // Aux carry is not used in space invaders, pending implementation
    ResultCodes.ac = 1;

    return ResultCodes;
}

uint8_t CPU::FlagCalc(CPU::FlagCodes flagState)
{
    uint8_t FlagValue;
    // calculate Flag register value based on:
    //   |7 |6 |5 |4 |3 |2 |1 |0 |
    //   |s |z |- |ac|- |p |- |cy|
    FlagValue = flagState.cy + (flagState.p * 4) + (flagState.ac * 16) + (flagState.z * 64) + (flagState.s * 128);
    return FlagValue;
}

bool CPU::IsAuxFlagSet(uint16_t number)
{
    return true;
}

void CPU::PerformInterrupt(State8080 *state)
{
    if (state->int_enable)
    {
        state->halted = false;
        uint16_t valuePC = state->pc - 1;
        uint8_t upperByte = uint8_t(valuePC >> 8);
        uint8_t lowerByte = uint8_t(valuePC - (upperByte << 8));

        // push statepc PUSH PC - seperate into upper and lower then set lower to sp - 2 and upper to sp - 1
        state->mem[state->sp - 2] = lowerByte;
        state->mem[state->sp - 1] = upperByte;
        state->pc = 8 * interruptNumber;
        state->sp -= 2;
        if (interruptNumber == 1)
        {
            interruptNumber = 2;
        }
        else
        {
            interruptNumber = 1;
        }
        state->int_enable = 0;
    }
}

void CPU::HandleInput(State8080 *state, uint8_t port)
{
    switch (port)
    {
    case 1:
        state->a = state->port1;
        break;
    case 2:
        state->a = state->port2;
        break;
    case 3:
    {
        uint16_t v = (shift1 << 8) | shift0;
        state->a = ((v >> (8 - shift_offset)) & 0xff);
    }
    break;
    }
}

void CPU::HandleOutput(uint8_t port, uint8_t value, State8080 *state)
{
    switch (port)
    {
    case 2:
        shift_offset = value & 0x7;
        break;
    case 3:
        state->out_port3 = value;
        break;
    case 4:
        shift0 = shift1;
        shift1 = value;
        break;
    case 5:
        state->out_port5 = value;
        break;
    }
}

void CPU::AudioBootup(){
    Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);
}

void CPU::AudioTearDown() {
    //Mix_FreeChunk to get rid of sound effect
    Mix_CloseAudio();
}


void CPU::PlayAudio(State8080 *state)
{
    if(state->out_port3 != state->out_port3_prev){
        //UFO sound
        if((state->out_port3 & 0x1) && !(state->out_port3_prev & 0x1)) {
            Mix_PlayChannel(1, Mix_LoadWAV("sounds/0.wav"), -1);
        }

        else if(!(state->out_port3 & 0x1) && (state->out_port3_prev & 0x1)){
            Mix_HaltChannel(1);
        }
        //player shooting
        if((state->out_port3 & 0x2) && !(state->out_port3_prev & 0x2)){
            Mix_PlayChannel(2, Mix_LoadWAV("sounds/1.wav"), 0);
        }

        //player dying
        if((state->out_port3 & 0x4) && !(state->out_port3_prev & 0x4)){
            Mix_PlayChannel(3, Mix_LoadWAV("sounds/2.wav"), 0);
        }

        //Invader dying
        if((state->out_port3 & 0x8) && !(state->out_port3_prev & 0x8)){
            Mix_PlayChannel(4, Mix_LoadWAV("sounds/3.wav"), 0);
        }
        state->out_port3_prev = state->out_port3;
    }

    if(state->out_port5 != state->out_port5_prev){
        //Invader beepboop #1
        if((state->out_port5 & 0x1) && !(state->out_port5_prev & 0x1)){
            Mix_PlayChannel(5, Mix_LoadWAV("sounds/4.wav"), 0);
        }

        //Invader beepboop #2
        if((state->out_port5 & 0x2) && !(state->out_port5_prev & 0x2)){
            Mix_PlayChannel(6, Mix_LoadWAV("sounds/5.wav"), 0);
        }

        //Invader beepboop #3
        if((state->out_port5 & 0x4) && !(state->out_port5_prev & 0x4)){
            Mix_PlayChannel(7, Mix_LoadWAV("sounds/6.wav"), 0);
        }

        //Invader beepboop #4 (?)
        if((state->out_port5 & 0x8) && !(state->out_port5_prev & 0x8)){
            Mix_PlayChannel(8, Mix_LoadWAV("sounds/7.wav"), 0);
        }
    }
}

// Function for emulating 8080 opcodes, has case for each of our opcodes
// Unimplemented instructions will call UnimplementedInstruction function
int CPU::Emulate8080Codes(State8080 *state)
{
    unsigned char *opcode = &state->mem[state->pc];
    // print the opcode before executing
    // Disassemble8080Op(state->mem, state->pc);
    uint32_t result;
    uint8_t upperdec;
    uint8_t lowerdec;

    // Combined registers
    // needs to be calculated at usage time
    uint16_t bc;
    uint16_t hl;
    uint16_t de;

    switch (*opcode)
    {
    case 0x00:
        break; // NOP

    case 0x01: // LXI B,D16
        state->b = opcode[2];
        state->c = opcode[1];
        state->pc += 2;
        break;

    case 0x02: // STAX B
        // to create bc shift bits left by 8, creating an empty 8 right bits.
        // Add state->c to the right bits with bitwise or.
        bc = (state->b << 8) | state->c;
        state->mem[bc] = state->a;
        break;

    case 0x03: // INX B
        bc = (state->b << 8) | state->c;
        bc += 1;
        state->b = bc >> 8;                // delete the last 8 bits to get b alone
        state->c = (bc - (state->b << 8)); // subtract the first 8 bits to get c alone
        break;

    case 0x04: // INR B
        state->b += 1;
        result = state->b;
        state->f = SetFlags(result);
        break;

    case 0x05: // DCR B
        state->b -= 1;
        result = state->b;
        state->f = SetFlags(result);
        break;

    case 0x06: // MVI B, D8
        state->b = opcode[1];
        state->pc += 1;
        break;

    case 0x07:                              // RLC
        result = 0x80 == (state->a & 0x80); // get most significant bit
        state->a = state->a << 1;
        if (result)
            state->a += 1; // Low order bit set to value shifted out of high order bit
        state->f.cy = result;
        break;

    case 0x08: // NOP
        break;

    case 0x09: // DAD B
        bc = (state->b << 8) | state->c;
        hl = (state->h << 8) | state->l;
        result = bc + hl;
        hl = result & 0xffff;
        state->h = hl >> 8;
        state->l = (hl - (state->h << 8));
        state->f.cy = result > 0xffff;
        break;

    case 0x0A: // LDAX B
        bc = (state->b << 8) | state->c;
        state->a = state->mem[bc];
        break;

    case 0x0B: // DCX B
        bc = (state->b << 8) | state->c;
        bc -= 1;
        state->b = bc >> 8;
        state->c = (bc - (state->b << 8));
        break;

    case 0x0C: // INR C
        state->c += 1;
        state->f.z = state->c == 0;
        state->f.s = 0x80 == (state->c & 0x80);
        state->f.p = Parity(state->c);
        state->f.ac = 1;
        break;

    case 0x0D: // DCR C
        state->c -= 1;
        state->f.z = state->c == 0;
        state->f.s = 0x80 == (state->c & 0x80);
        state->f.p = Parity(state->c);
        state->f.ac = 1;
        break;

    case 0x0E: // MVI C,D8
        state->c = opcode[1];
        state->pc += 1;
        break;

    case 0x0F:                 // RRC
        result = state->a & 1; // get least significant bit
        state->a = state->a >> 1;
        if (result)
            state->a = state->a | 0x80; // Low order bit set to value shifted out of high order bit
        state->f.cy = result;
        break;

    case 0x10: // NOP
        break;

    case 0x11: // LXI D,D16
        state->d = opcode[2];
        state->e = opcode[1];
        state->pc += 2;
        break;

    case 0x12: // STAX D
        de = (state->d << 8) | state->e;
        state->mem[de] = state->a;
        break;

    case 0x13: // INX D
        de = (state->d << 8) | state->e;
        de += 1;
        state->d = de >> 8;
        state->e = (de - (state->d << 8));
        break;

    case 0x14: // INR D
        state->d += 1;
        state->f.z = state->d == 0;
        state->f.s = 0x80 == (state->d & 0x80);
        state->f.p = Parity(state->d);
        state->f.ac = (state->d & 0x0F) == 0x00; // Sets AC if lower nibble overflows
        break;

    case 0x15: // DCR D
        state->d -= 1;
        state->f.z = state->d == 0;
        state->f.s = 0x80 == (state->d & 0x80);
        state->f.p = Parity(state->d);
        state->f.ac = (state->d & 0x0F) == 0x0F; // Sets AC if lower nibble has to borrow from bit 4
        break;

    case 0x16: // MVI D, d8
        state->d = opcode[1];
        state->pc += 1;
        break;

    case 0x17:                              // RAL
        result = 0x80 == (state->a & 0x80); // Stores highest bit
        state->a = state->a << 1;

        if (state->f.cy)
        {
            state->a = state->a | 1 << 0;
        }

        state->f.cy = result;
        break;

    case 0x18: // NOP
        break;

    case 0x19: // DAD D
        de = (state->d << 8) | state->e;
        hl = (state->h << 8) | state->l;
        result = de + hl;
        hl = result & 0xffff;
        state->h = hl >> 8;
        state->l = (hl - (state->h << 8));
        state->f.cy = result > 0xffff;
        break;

    case 0x1A: // LDAX D
        de = (state->d << 8) | state->e;
        state->a = state->mem[de];
        break;

    case 0x1B: // DCX D
        de = (state->d << 8) | state->e;
        de -= 1;
        state->d = de >> 8;
        state->e = (de - (state->d << 8));
        break;

    case 0x1C: // INR E
        state->e += 1;
        state->f.z = state->e == 0;
        state->f.s = 0x80 == (state->e & 0x80);
        state->f.p = Parity(state->e);
        state->f.ac = (state->e & 0x0F) == 0x00; // Sets AC if lower nibble overflows
        break;

    case 0x1D: // DCR E
        state->e -= 1;
        state->f.z = state->e == 0;
        state->f.s = 0x80 == (state->e & 0x80);
        state->f.p = Parity(state->e);
        state->f.ac = (state->e & 0x0F) == 0x0F; // Sets AC if lower nibble has to borrow from bit 4
        break;

    case 0x1E: // MVI E, d8
        state->e = opcode[1];
        state->pc += 1;
        break;

    case 0x1F:                              // RAR
        result = 0x01 == (state->a & 0x01); // Stores lowest bit
        state->a = state->a >> 1;

        if (state->f.cy)
        {
            state->a = state->a | 1 << 7;
        }
        state->f.cy = result;
        break;

    case 0x20: // NOP
        break;

    case 0x21: // LXI H, d16
        state->h = opcode[2];
        state->l = opcode[1];
        state->pc += 2;
        break;

    case 0x22: // SHLD a16
        result = (opcode[2] << 8) | opcode[1];
        state->mem[result] = state->l;
        result += 1;
        state->mem[result] = state->h;
        state->pc += 2;
        break;

    case 0x23: // INX H
        hl = (state->h << 8) | state->l;
        hl += 1;
        state->h = hl >> 8;
        state->l = (hl - (state->h << 8));
        break;

    case 0x24: // INR H
        state->h += 1;
        state->f.z = state->h == 0;
        state->f.s = 0x80 == (state->h & 0x80);
        state->f.p = Parity(state->h);
        state->f.ac = (state->h & 0x0F) == 0x00; // Sets AC if lower nibble overflows
        break;

    case 0x25: // DCR H
        state->h -= 1;
        state->f.z = state->h == 0;
        state->f.s = 0x80 == (state->h & 0x80);
        state->f.p = Parity(state->h);
        state->f.ac = (state->h & 0x0F) == 0x0F; // Sets AC if lower nibble has to borrow from bit 4
        break;

    case 0x26: // MVI H, d8
        state->h = opcode[1];
        state->pc += 1;
        break;

    case 0x27: // DAA (DEPENDENT ON AC FLAG, MAKE SURE AC FLAG SET UP RIGHT)
        lowerdec = state->a & 0x0F;

        if (lowerdec > 9 || state->f.ac == 1)
        {
            state->a += 6;
            state->f.ac = lowerdec > 15;
        }

        upperdec = state->a >> 4;
        lowerdec = state->a & 0x0F;

        if (upperdec > 9 || state->f.cy == 1)
        {
            upperdec += 6;
            state->f.cy = upperdec > 15;
        }

        state->a = upperdec << 4 | lowerdec;
        state->f.p = Parity(state->a);
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->h & 0x80);
        break;

    case 0x28:
        break; // NOP

    case 0x29: // DAD H
        hl = (state->h << 8) | state->l;
        result = hl + hl;
        state->f.cy = (result > 0xffff);
        hl = result & 0xffff; // we need to strip off any overflow bits
        state->h = hl >> 8;
        state->l = (hl - (state->h << 8));
        break;

    case 0x2A: // LHLD adr
        result = (opcode[2] << 8) | opcode[1];
        state->l = state->mem[result];
        state->h = state->mem[result + 1];
        state->pc += 2;
        break;

    case 0x2B: // DCX H
        hl = (state->h << 8) | state->l;
        hl -= 1;
        state->h = hl >> 8;
        state->l = (hl - (state->h << 8));
        break;

    case 0x2C: // INR L
        state->l += 1;
        state->f.p = Parity(state->l);
        state->f.z = 0 == state->l;
        state->f.s = 0x80 == (state->l & 0x80);
        state->f.ac = 1;
        break;

    case 0x2D: // DCR L
        state->l -= 1;
        state->f.p = Parity(state->l);
        state->f.z = 0 == state->l;
        state->f.s = 0x80 == (state->l & 0x80);
        state->f.ac = 1;
        break;

    case 0x2E: // MVI L, D8
        state->l = opcode[1];
        state->pc += 1;
        break;

    case 0x2F:                        // CMA
        state->a = state->a ^ 0xffff; // inverse bits using XOR
        break;

    case 0x30: // NOP
        break;

    case 0x31: // LXI SP, D16
        state->sp = (opcode[2] << 8) | opcode[1];
        state->pc += 2;
        break;

    case 0x32: // STA adr
        result = (opcode[2] << 8) | opcode[1];
        state->mem[result] = state->a;
        state->pc += 2;
        break;

    case 0x33: // INX SP
        state->sp += 1;
        break;

    case 0x34: // INR M
        hl = (state->h << 8) | state->l;
        state->mem[hl] = state->mem[hl] + 1;
        state->f.p = Parity(state->mem[hl]);
        state->f.z = 0 == state->mem[hl];
        state->f.s = 0x8000 == (state->mem[hl] & 0x8000);
        state->f.ac = 1;
        break;

    case 0x35: // DCR M
        hl = (state->h << 8) | state->l;
        state->mem[hl] = state->mem[hl] - 1;
        state->f.p = Parity(state->mem[hl]);
        state->f.z = 0 == state->mem[hl];
        state->f.s = 0x8000 == (state->mem[hl] & 0x8000);
        state->f.ac = 1;
        break;

    case 0x36: // MVI M,D8
        hl = (state->h << 8) | state->l;
        state->mem[hl] = opcode[1];
        state->pc += 1;
        break;

    case 0x37: // STC
        state->f.cy = 1;
        break;

    case 0x38: // NOP
        break;

    case 0x39: // DAD SP
        hl = (state->h << 8) | state->l;
        result = hl + state->sp;
        state->f.cy = (hl > 0xffff);
        hl = result & 0xffff;
        state->h = hl >> 8;
        state->l = (hl - (state->h << 8));
        break;

    case 0x3A: // LDA adr
        result = (opcode[2] << 8) | opcode[1];
        state->a = state->mem[result];
        state->pc += 2;
        break;

    case 0x3B: // DCX SP
        state->sp -= 1;
        break;

    case 0x3C:
        // INR A
        state->a += 1;
        result = state->a;
        state->f = SetFlags(result);
        break;

    case 0x3D:
        // DCR A
        state->a -= 1;
        result = state->a;
        state->f = SetFlags(result);
        break;

    case 0x3E:
        // MVI A, 0x%02x, code[1] -- 2 opBytes
        state->a = opcode[1];
        state->pc += 1;
        break;

    case 0x3F:
        // CMC
        // Complement of the carry flag. Flips the carry bit of the flags register
        state->f.cy = (!state->f.cy);
        break;

    case 0x40:
        // MOV B, B
        state->b = state->b;
        break;

    case 0x41:
        // MOV B, C
        state->b = state->c;
        break;

    case 0x42:
        // MOV B, D
        state->b = state->d;
        break;

    case 0x43:
        // MOV B, E
        state->b = state->e;
        break;

    case 0x44:
        // MOV B, H
        state->b = state->h;
        break;

    case 0x45:
        // MOV B, L
        state->b = state->l;
        break;

    case 0x46:
        // MOV B, M
        hl = (state->h << 8) | state->l;
        state->b = state->mem[hl];
        break;

    case 0x47:
        // MOV B, A
        state->b = state->a;
        break;

    case 0x48:
        // MOV C, B
        state->c = state->b;
        break;

    case 0x49:
        // MOV C, C
        state->c = state->c;
        break;

    case 0x4A:
        // MOV C, D
        state->c = state->d;
        break;

    case 0x4B:
        // MOV C, E
        state->c = state->e;
        break;

    case 0x4C:
        // MOV C, H
        state->c = state->h;
        break;

    case 0x4D:
        // MOV C, L
        state->c = state->l;
        break;

    case 0x4E:
        // MOV C, M
        hl = (state->h << 8) | state->l;
        state->c = state->mem[hl];
        break;

    case 0x4F:
        // MOV C, A
        state->c = state->a;
        break;

    case 0x50:
        state->d = state->b;
        break;

    case 0x51:
        state->d = state->c;
        break;

    case 0x52:
        // state->d = state->d; This operation does nothing MOV D, D
        break;

    case 0x53:
        state->d = state->e;
        break;

    case 0x54:
        state->d = state->h;
        break;

    case 0x55:
        state->d = state->l;
        break;

    case 0x56:
        // MOV D,M moves the number stored in the address at HL to register D
        // shift H left by 8 bits and do an or operator with L
        hl = (state->h << 8) | (state->l);
        state->d = state->mem[hl];
        break;

    case 0x57:
        state->d = state->a;
        break;

    case 0x58:
        state->e = state->b;
        break;

    case 0x59:
        state->e = state->c;
        break;

    case 0x5A:
        state->e = state->d;
        break;

    case 0x5B:
        // state->e = state->e; this code does nothing
        break;

    case 0x5C:
        state->e = state->h;
        break;

    case 0x5D:
        state->e = state->l;
        break;

    case 0x5E:
        hl = (state->h << 8) | (state->l);
        state->e = state->mem[hl];
        break;

    case 0x5F:
        state->e = state->a;
        break;

    case 0x60:
        state->h = state->b;
        break;

    case 0x61:
        state->h = state->c;
        break;

    case 0x62:
        state->h = state->d;
        break;

    case 0x63:
        state->h = state->e;
        break;

    case 0x64:
        state->h = state->h;
        break;

    case 0x65:
        // mov h,l
        state->h = state->l;
        break;

    case 0x66:
        // mov h,m
        hl = (state->h << 8) | (state->l);
        state->h = state->mem[hl];
        break;

    case 0x67:
        // mov h,a
        state->h = state->a;
        break;

    case 0x68:
        // mov l,b
        state->l = state->b;
        break;

    case 0x69:
        // mov l,c
        state->l = state->c;
        break;

    case 0x6A:
        // mov l,d
        state->l = state->d;
        break;

    case 0x6B:
        // mov l,e
        state->l = state->e;
        break;

    case 0x6C:
        // mov l,h
        state->l = state->h;
        break;

    case 0x6D:
        // mov l,l
        state->l = state->l;
        break;

    case 0x6E:
        // mov l,m
        hl = (state->h << 8) | (state->l);
        state->l = state->mem[hl];
        break;

    case 0x6F:
        // mov l,a
        state->l = state->a;
        break;

    case 0x70:
        // mov m,b  (hl)<-b
        hl = (state->h << 8) | (state->l);
        state->mem[hl] = state->b;
        break;

    case 0x71:
        // mov m,c  (hl)<-c
        hl = (state->h << 8) | (state->l);
        state->mem[hl] = state->c;
        break;

    case 0x72:
        // mov m,d  (hl)<-d
        hl = (state->h << 8) | (state->l);
        state->mem[hl] = state->d;
        break;

    case 0x73:
        // mov m,e  (hl)<-e
        hl = (state->h << 8) | (state->l);
        state->mem[hl] = state->e;
        break;

    case 0x74:
        // mov m,h   (hl)<-h
        hl = (state->h << 8) | (state->l);
        state->mem[hl] = state->h;
        break;

    case 0x75:
        // mov m,l  (hl)<-l
        hl = (state->h << 8) | (state->l);
        state->mem[hl] = state->l;
        break;

    case 0x76:
        state->halted = true;
        break;

    case 0x77:
        // mov m,a  (hl)<-a     error in opcodes page?
        hl = (state->h << 8) | (state->l);
        state->mem[hl] = state->a;
        break;

    case 0x78:
        // mov a,b    b<-a
        state->a = state->b;
        break;

    case 0x79: // MOV A, C
        state->a = state->c;
        break;

    case 0x7A: // MOV A, D
        state->a = state->d;
        break;

    case 0x7B: // MOV A, E
        state->a = state->e;
        break;

    case 0x7C: // MOV A, H
        state->a = state->h;
        break;

    case 0x7D: // MOV A, L
        state->a = state->l;
        break;

    case 0x7E: // MOV A, M
        hl = (state->h << 8) | state->l;
        state->a = state->mem[hl];
        break;

    case 0x7F: // MOV A, A
        state->a = state->a;
        break;

    case 0x80:                      // ADD B
        lowerdec = state->a & 0x0F; // Pulls register A low 4 bit nibble
        upperdec = state->b & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + state->b;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x81:                      // ADD C
        lowerdec = state->a & 0x0F; // Pulls register A low 4 bit nibble
        upperdec = state->c & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + state->c;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x82:                      // ADD D
        lowerdec = state->a & 0x0F; // Pulls register A low 4 bit nibble
        upperdec = state->d & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + state->d;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x83:                      // ADD E
        lowerdec = state->a & 0x0F; // Pulls register A low 4 bit nibble
        upperdec = state->e & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + state->e;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x84:                      // ADD H
        lowerdec = state->a & 0x0F; // Pulls register A low 4 bit nibble
        upperdec = state->h & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + state->h;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x85:                      // ADD L
        lowerdec = state->a & 0x0F; // Pulls register A low 4 bit nibble
        upperdec = state->l & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + state->l;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x86: // ADD M
        hl = (state->h << 8) | state->l;
        lowerdec = state->a & 0x0F;       // Pulls register A low 4 bit nibble
        upperdec = state->mem[hl] & 0x0F; // Pulls mem value low 4 bit nibble

        result = state->a + state->mem[hl];
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x87:                      // ADD A
        lowerdec = state->a & 0x0F; // Pulls register A low 4 bit nibble
        upperdec = state->a & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + state->a;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x88:                                      // ADC B
        lowerdec = state->a & 0x0F;                 // Pulls register A low 4 bit nibble
        upperdec = (state->b & 0x0F) + state->f.cy; // Pulls added register low 4 bit nibble, adds CY value

        result = state->a + state->b + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x89:                                      // ADC C
        lowerdec = state->a & 0x0F;                 // Pulls register A low 4 bit nibble
        upperdec = (state->c & 0x0F) + state->f.cy; // Pulls added register low 4 bit nibble, adds CY value

        result = state->a + state->c + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x8A:                                      // ADC D
        lowerdec = state->a & 0x0F;                 // Pulls register A low 4 bit nibble
        upperdec = (state->d & 0x0F) + state->f.cy; // Pulls added register low 4 bit nibble, adds CY value

        result = state->a + state->d + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x8B:                                      // ADC E
        lowerdec = state->a & 0x0F;                 // Pulls register A low 4 bit nibble
        upperdec = (state->e & 0x0F) + state->f.cy; // Pulls added register low 4 bit nibble, adds CY value

        result = state->a + state->e + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x8C:                                      // ADC H
        lowerdec = state->a & 0x0F;                 // Pulls register A low 4 bit nibble
        upperdec = (state->h & 0x0F) + state->f.cy; // Pulls added register low 4 bit nibble, adds CY value

        result = state->a + state->h + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x8D:                                      // ADC L
        lowerdec = state->a & 0x0F;                 // Pulls register A low 4 bit nibble
        upperdec = (state->l & 0x0F) + state->f.cy; // Pulls added register low 4 bit nibble, adds CY value

        result = state->a + state->l + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x8E: // ADC M
        hl = (state->h << 8) | state->l;
        lowerdec = state->a & 0x0F;                       // Pulls register A low 4 bit nibble
        upperdec = (state->mem[hl] & 0x0F) + state->f.cy; // Pulls added mem low 4 bit nibble, adds CY value

        result = state->a + state->mem[hl] + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x8F:                                      // ADC A
        lowerdec = state->a & 0x0F;                 // Pulls register A low 4 bit nibble
        upperdec = (state->a & 0x0F) + state->f.cy; // Pulls added register low 4 bit nibble, adds CY value

        result = state->a + state->a + state->f.cy;
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        break;

    case 0x90:                                                               // SUB B
        result = state->a + (~state->b) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->b) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->b > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x91:                                                               // SUB C
        result = state->a + (~state->c) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->c) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->c > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x92:                                                               // SUB D
        result = state->a + (~state->d) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->d) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->d > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x93:                                                               // SUB E
        result = state->a + (~state->e) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->e) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->e > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x94:                                                               // SUB H
        result = state->a + (~state->h) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->h) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->h > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x95:                                                               // SUB L
        result = state->a + (~state->l) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->l) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->l > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x96: // SUB M
        hl = (state->h << 8) | state->l;
        result = state->a + (~state->mem[hl]) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->mem[hl]) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->mem[hl] > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x97:                                                               // SUB A
        result = state->a + (~state->a) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->a) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = 0;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x98:                                                                             // SBB B
        result = state->a + ~(state->b + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->b + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->b + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->b + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x99:                                                                             // SBB C
        result = state->a + ~(state->c + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->c + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->c + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->c + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x9A:                                                                             // SBB D
        result = state->a + ~(state->d + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->d + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->d + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->d + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x9B:                                                                             // SBB E
        result = state->a + ~(state->e + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->e + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->e + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->e + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x9C:                                                                             // SBB H
        result = state->a + ~(state->h + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->h + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->h + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->h + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;
    case 0x9D:                                                                             // SBB L
        result = state->a + ~(state->l + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->l + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->l + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->l + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x9E: // SBB M
        hl = (state->h << 8) | state->l;
        result = state->a + ~(state->mem[hl] + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->mem[hl] + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->mem[hl] + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->mem[hl] + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0x9F:                                                                             // SBB A
        result = state->a + ~(state->a + state->f.cy) + 1;                                 // 2s complement subtraction, flips state of CY flag
        state->f.ac = ((state->a & 0x0F) + (~(state->a + state->f.cy) + 1 & 0x0F) > 0x0F); // Apparently they don't bother flipping this
        state->f.cy = (state->a + state->f.cy) > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = state->a == (state->a + state->f.cy);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;
        break;

    case 0xA0:                      // ANA B
        lowerdec = state->a & 0x08; // Isolate bit 3 from A register
        upperdec = state->b & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->b;
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA1:                      // ANA C
        lowerdec = state->a & 0x08; // Isolate bit 3 from A register
        upperdec = state->c & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->c;
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA2:                      // ANA D
        lowerdec = state->a & 0x08; // Isolate bit 3 from A register
        upperdec = state->d & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->d;
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA3:                      // ANA E
        lowerdec = state->a & 0x08; // Isolate bit 3 from A register
        upperdec = state->e & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->e;
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA4:                      // ANA H
        lowerdec = state->a & 0x08; // Isolate bit 3 from A register
        upperdec = state->h & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->h;
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA5:                      // ANA L
        lowerdec = state->a & 0x08; // Isolate bit 3 from A register
        upperdec = state->l & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->l;
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA6: // ANA M
        hl = (state->h << 8) | state->l;
        lowerdec = state->a & 0x08;       // Isolate bit 3 from A register
        upperdec = state->mem[hl] & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->mem[hl];
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA7:                      // ANA A
        lowerdec = state->a & 0x08; // Isolate bit 3 from A register
        upperdec = state->a & 0x08; // Isolate bit 3 from AND register

        state->f.cy = 0;                             // ANA clears carry
        state->f.ac = 0x08 == (lowerdec | upperdec); // AC flag set to OR of bit 3s from involved registers
        state->a = state->a & state->a;
        state->f.z = 0 == state->a;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.p = Parity(state->a);
        break;

    case 0xA8: // XRA B
        state->a = state->a ^ state->b;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xA9: // XRA C
        state->a = state->a ^ state->c;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xAA: // XRA D
        state->a = state->a ^ state->d;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xAB: // XRA E
        state->a = state->a ^ state->e;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xAC: // XRA H
        state->a = state->a ^ state->h;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xAD: // XRA L
        state->a = state->a ^ state->l;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xAE: // XRA M
        hl = (state->h << 8) | state->l;
        state->a = state->a ^ state->mem[hl];
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xAF: // XRA A
        state->a = state->a ^ state->a;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB0: // ORA B
        state->a = state->a | state->b;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB1: // ORA C
        state->a = state->a | state->c;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB2: // ORA D
        state->a = state->a | state->d;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB3: // ORA E
        state->a = state->a | state->e;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB4: // ORA H
        state->a = state->a | state->h;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB5: // ORA L
        state->a = state->a | state->l;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB6: // ORA M
        hl = (state->h << 8) | state->l;
        state->a = state->a | state->mem[hl];
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB7: // ORA A
        state->a = state->a | state->a;
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        break;

    case 0xB8:                                                               // CMP B
        result = state->a + (~state->b) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->b) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->b > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xB9:                                                               // CMP C
        result = state->a + (~state->c) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->c) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->c > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xBA:                                                               // CMP D
        result = state->a + (~state->d) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->d) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->d > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xBB:                                                               // CMP E
        result = state->a + (~state->e) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->e) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->e > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xBC:                                                               // CMP H
        result = state->a + (~state->h) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->h) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->h > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xBD:                                                               // CMP L
        result = state->a + (~state->l) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->l) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->l > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xBE: // CMP M
        hl = (state->h << 8) | state->l;
        result = state->a + (~state->mem[hl]) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->mem[hl]) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = state->mem[hl] > state->a;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xBF:                                                               // CMP A
        result = state->a + (~state->a) + 1;                                 // 2s complement subtraction
        state->f.ac = ((state->a & 0x0F) + ((~state->a) & 0x0F) + 1) > 0x0F; // Apparently they don't bother flipping this
        state->f.cy = 0;
        state->f.s = 0x80 == (result & 0x80);
        state->f.z = 0 == (result & 0xFF);
        state->f.p = Parity(result & 0xFF);
        break;

    case 0xC0: // RNZ
        if (!(state->f.z))
        {
            state->pc = state->mem[state->sp] | (state->mem[state->sp + 1] << 8);
            state->sp += 2;
        }
        break;

    case 0xC1: // POP B
        state->c = state->mem[state->sp];
        state->b = state->mem[state->sp + 1];
        state->sp += 2;
        break;

    case 0xC2: // JNZ a16
        if (!(state->f.z))
        {
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xC3: // JMP a16
        state->pc = (opcode[2] << 8) | opcode[1];
        state->pc--;
        break;

    case 0xC4: // CNZ a16
        if (!(state->f.z))
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xC5: // PUSH B
        state->mem[state->sp - 1] = state->b;
        state->mem[state->sp - 2] = state->c;
        state->sp -= 2;
        break;

    case 0xC6:                       // ADI d8
        lowerdec = state->a & 0x0F;  // Pulls register A low 4 bit nibble
        upperdec = opcode[1] & 0x0F; // Pulls added register low 4 bit nibble

        result = state->a + opcode[1];
        state->f.z = (0 == (result & 0xFF)); // Accounts for potential carry out of range of register A
        state->f.s = (0x80 == (result & 0x80));
        state->f.cy = (result > 0xFF);
        state->f.p = Parity(result & 0xFF);
        state->a = result & 0xFF;

        result = lowerdec + upperdec; // Add lower 4 bit pairs to see if carry from bit 3 into bit 4
        state->f.ac = result > 15;    // If result greater 4 bit capacity, set AC flag, clears otherwise
        state->pc++;
        break;

    case 0xC7: // RST 0
        result = state->pc;
        state->mem[state->sp - 1] = (result >> 8);
        state->mem[state->sp - 2] = (result & 0xFF);
        state->sp -= 2;
        state->pc = 0xFFFF; // Set up to overflow to 0x0000 with end of statement increment
        break;

    case 0xC8: // RZ
        if (state->f.z)
        {
            state->pc = state->mem[state->sp] | (state->mem[state->sp + 1] << 8);
            state->sp += 2;
        }
        break;

    case 0xC9: // RET
        state->pc = state->mem[state->sp] | (state->mem[state->sp + 1] << 8);
        state->sp += 2;
        break;

    case 0xCA: // JZ a16
        if (state->f.z)
        {
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xCB: // JMP a16
        state->pc = (opcode[2] << 8) | opcode[1];
        state->pc--;
        break;

    case 0xCC: // CZ a16
        if (state->f.z)
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xCD:                                     // CALL a16
        result = state->pc + 2;                    // save the address of the next instruction
        state->mem[state->sp - 1] = (result >> 8); // high-order bits in higher stack addr
        state->mem[state->sp - 2] = result & 0xff; // low-order bits in lower stack addr
        state->sp -= 2;                            // stack grows downward
        state->pc = (opcode[2] << 8) | opcode[1];  // Jump to the address immediately after the pc
        state->pc--;

        break;

    case 0xCE:                                       // ACI d8
        result = state->a + opcode[1] + state->f.cy; // add accumulator, immediate byte and carry flag
        state->f = SetFlags(result);                 // set flags
        state->a = result & 0xff;                    // store lower 8 bits in register A
        state->pc++;                                 // increment the pointer
        break;

    case 0xCF:                                     // RST1
        result = state->pc;                        // save the address of the next instruction
        state->mem[state->sp - 1] = (result >> 8); // high-order bits in higher stack addr
        state->mem[state->sp - 2] = result & 0xff; // low-order bits in lower stack addr
        state->sp -= 2;                            // stack grows downward
        state->pc = 0x0008;                        // sets pc to 8 multiplied by the number associated with RST (8*1)
        state->pc--;
        break;

    case 0xD0: // RNC
        if (!state->f.cy)
        {
            result = (state->mem[state->sp + 1] << 8) | state->mem[state->sp]; // Construct the return address from Stack
            state->pc = result;                                                // Jump to the return address
            state->sp += 2;                                                    // shorten the stack
        }
        break;

    case 0xD1:                                // POP D
        state->d = state->mem[state->sp + 1]; // high-addr bits in higher order register
        state->e = state->mem[state->sp];     // low-addr bits in lower order register
        state->sp += 2;                       // shorten the stack
        break;

    case 0xD2: // JNC a16
        if (!state->f.cy)
        {
            result = (opcode[2] << 8) | opcode[1]; // retrieve address from immediate data
            state->pc = result;                    // jump pc to address
            state->pc--;
        }
        else
        {
            state->pc += 2; // if the carry flag is set. move to the next instruction
        }
        break;

    case 0xD3: // OUT d8
        // this is unimplemented as it deals with sending data to external hardware
        // for now I'll skip over the operation's data
        HandleOutput(opcode[1], state->a, state);
        PlayAudio(state);
        state->pc++;
        break;

    case 0xD4: // CNC
        if (!state->f.cy)
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2; // if the carry flag is set, move pc to the next instruction
        }
        break;

    case 0xD5:                                // PUSH D
        state->mem[state->sp - 1] = state->d; // higher register bits to the higher sp
        state->mem[state->sp - 2] = state->e; // lower register bits to the lower sp
        state->sp -= 2;
        break;

    case 0xD6:                              // SUI d8
        result = state->a + ~opcode[1] + 1; // two's compliment subtraction
        state->f = SetFlags(result);        // set the flags
        state->a = result & 0xff;           // store the bottom 8 bits
        state->pc++;
        break;

    case 0xD7:                                     // RST 2
        result = state->pc;                        // Store the address of the next instruction on the stack
        state->mem[state->sp - 1] = (result >> 8); // store the higher bits of the addr in the higher stack addr
        state->mem[state->sp - 2] = result & 0xff; // store the lower bits of the address in the lower stack addr
        state->sp -= 2;                            // stack grows downward
        state->pc = 0x0010;                        // sets pc to 8 multiplied by the number associated with RST (8*2)
        state->pc--;
        break;

    case 0xD8: // RC
        if (state->f.cy)
        {
            result = (state->mem[state->sp + 1] << 8) | state->mem[state->sp]; // load address from the stack
            state->pc = result;
            state->sp += 2;
        }
        break;

    case 0xD9:                                                             //*RET
        result = (state->mem[state->sp + 1] << 8) | state->mem[state->sp]; // load address from the stack
        state->pc = result;
        state->sp += 2;
        break;

    case 0xDA: // JC a16
        if (state->f.cy)
        {
            result = (opcode[2] << 8) | opcode[1];
            state->pc = result;
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xDB: // IN d8
        HandleInput(state, opcode[1]);
        state->pc++; // Since we haven't gotten to the I/O part of our project, this just skips over the data
        break;

    case 0xDC: // CC A16
        if (state->f.cy)
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xDD:                                     //*Call a16
        result = state->pc + 2;                    // push the address of the next instruction to the stack
        state->mem[state->sp - 1] = (result >> 8); // higher 8 bits to the higher sp
        state->mem[state->sp - 2] = result & 0xff; // lower 8 bits to the lower sp
        state->sp -= 2;                            // stack grows downward
        state->pc = (opcode[2] << 8) | opcode[1];  // jump to address loaded from immediate data
        state->pc--;
        break;

    case 0xDE:                                              // SBI d8
        result = state->a + ~(opcode[1] + state->f.cy) + 1; // twos compliment difference of the accumulator and the sum of immediate 8 bits and carry flag
        state->f = SetFlags(result);                        // set flags
        state->a = result & 0xff;                           // only store the least significant 8 bits
        state->pc++;                                        // Increment the pc past immediate data
        break;

    case 0xDF:                                     // RST 3
        result = state->pc;                        // Store the address of the next instruction on the stack
        state->mem[state->sp - 1] = (result >> 8); // store the higher bits of the addr in the higher stack addr
        state->mem[state->sp - 2] = result & 0xff; // store the lower bits of the address in the lower stack addr
        state->sp -= 2;                            // stack grows downward
        state->pc = 0x0018;                        // sets pc to 8 multiplied by the number associated with RST (8*3)
        state->pc--;
        break;

    case 0xE0: // RPO - Return if parity flag is odd (cleared)
        if (!state->f.p)
        {
            state->pc = state->mem[state->sp] | (state->mem[state->sp + 1] << 8);
            state->sp += 2;
        }
        break;

    case 0xE1: //  POP H
        state->l = state->mem[state->sp];
        state->h = state->mem[state->sp + 1];
        state->sp += 2;
        break;

    case 0xE2: // JPO adr code[2], code[1] - Jump if party flag is odd (cleared)
        if (!state->f.p)
        {
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xE3: // XTHL - exchange thing at sp with l and thing at sp+1 with h
        // decrement HL before we stack it
        hl = (state->h << 8) | state->l;
        hl--;
        state->h = hl >> 8;
        state->l = (hl - (state->h << 8));

        result = state->l;
        state->l = state->mem[state->sp];
        state->mem[state->sp] = result;
        result = state->h;
        state->h = state->mem[state->sp + 1];
        state->mem[state->sp + 1] = result;
        break;

    case 0xE4: // CPO adr code[2], code[1] - call if parity flag even
        if (!state->f.p)
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xE5: // PUSH H
        state->mem[state->sp - 1] = state->h;
        state->mem[state->sp - 2] = state->l;
        state->sp = state->sp - 2;
        break;

    case 0xE6: // ANI data code[1] - A and opcode[1]. Clears carry and aux carry flags
        state->a = state->a & opcode[1];
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        state->pc++;
        break;

    case 0xE7: // RST 4 - transfer control to address 8 * 4
        state->mem[state->sp - 1] = state->pc >> 8;
        state->mem[state->sp - 2] = state->pc & 0xff;
        state->sp = state->sp - 2;
        state->pc = 0x0020;
        state->pc--;
        break;

    case 0xE8: // RPE - Return if parity equal
        if (state->f.p)
        {
            state->pc = state->mem[state->sp] | (state->mem[state->sp + 1] << 8);
            state->sp += 2;
        }
        break;

    case 0xE9: // PCHL - Jump H and L indirect. Moves H and L to PC
        hl = (state->h << 8) | state->l;
        state->pc = hl;
        state->pc--; // decrement control pointer so it stays at hl
        break;

    case 0xEA: // JPE adr code[2], code[1] - jump if parity equal
        if (state->f.p)
        {
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xEB: // XCHG - exchange d & e with h & l registers
        result = state->h;
        state->h = state->d;
        state->d = result;
        result = state->l;
        state->l = state->e;
        state->e = result;
        break;

    case 0xEC: // CPE adr code[2], code[1] - call if parity flag even
        if (state->f.p)
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xED: // CALL adr code[2], code[1]
        result = state->pc + 2;
        state->mem[state->sp - 1] = (result >> 8) & 0xFF;
        state->mem[state->sp - 2] = (result & 0xFF);
        state->sp = state->sp - 2;
        state->pc = (opcode[2] << 8) | opcode[1];
        state->pc--;
        break;

    case 0xEE: // XRI data code[1] - exclusive or A with opcode[1]. Clears carry and aux carry flags
        state->a = state->a ^ opcode[1];
        if (Parity(state->a))
        {
            state->f.p = 1; // set parity flag if 0th bit is 0
        }
        else
        {
            state->f.p = 0;
        }
        if (state->a == 0)
        {
            state->f.z = 1; // set zero flag if result is 0
        }
        else
        {
            state->f.z = 0;
        }
        if ((state->a & (1 << 7)) >> 7 == 1)
        {
            state->f.s = 1; // set sign flag if most significant bit is set
        }
        else
        {
            state->f.s = 0;
        }
        state->f.cy = 0;
        state->f.ac = 0;
        state->pc++;
        break;

    case 0xEF: // RST 5 - transfer control to address 8 * 5
        state->mem[state->sp - 1] = state->pc >> 8;
        state->mem[state->sp - 2] = state->pc & 0xff;
        state->sp = state->sp - 2;
        state->pc = 0x0028;
        state->pc--;
        break;

    case 0xF0: // RP - return if positive (sign flag is cleared)
        if (!state->f.s)
        {
            state->pc = state->mem[state->sp] | (state->mem[state->sp + 1] << 8);
            state->sp += 2;
        }
        break;

    case 0xF1: // POP PSW
        // Contents of memory location pointed at by SP is used to restore condition flags.
        // cy is 0th bit, p 2nd, ac 4th, z 6th, and s 7th.
        state->f.cy = (state->mem[state->sp] & (1 << 0)) >> 0;
        state->f.p = (state->mem[state->sp] & (1 << 2)) >> 2;
        state->f.ac = (state->mem[state->sp] & (1 << 4)) >> 4;
        state->f.z = (state->mem[state->sp] & (1 << 6)) >> 6;
        state->f.s = (state->mem[state->sp] & (1 << 7)) >> 7;
        state->a = state->mem[state->sp + 1]; // Then the datasheet says to do this
        state->sp = state->sp + 2;
        break;

    case 0xF2: // JP adr code[1], code[1] - jump if positive (sign flag is cleared)
        if (!state->f.s)
        {
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xF3: // DI - this disables system interrupts and would need to be implemented alongside later code
        state->int_enable = 0;
        break;

    case 0xF4: // CP adr
        if (state->f.s == 0)
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xF5: // PUSH PSW
        state->mem[state->sp - 2] = FlagCalc(state->f);
        state->mem[state->sp - 1] = state->a;
        state->sp -= 2;
        break;

    case 0xF6: // ORI D8 (carry bit reset to zero, Zero, Sign, and Parity set)
        state->a = state->a | opcode[1];
        state->f.cy = 0;
        state->f.ac = 0;
        state->f.s = 0x80 == (state->a & 0x80);
        state->f.z = 0 == (state->a & 0xFF);
        state->f.p = Parity(state->a & 0xFF);
        state->pc += 1;
        break;

    case 0xF7:                                     // RST 6
        result = state->pc;                        // Store the address of the next instruction on the stack
        state->mem[state->sp - 1] = (result >> 8); // store the higher bits of the addr in the higher stack addr
        state->mem[state->sp - 2] = result & 0xff; // store the lower bits of the address in the lower stack addr
        state->sp -= 2;                            // stack grows downward
        state->pc = 0x0030;                        // sets pc to 8 multiplied by the number associated with RST (8*6)
        state->pc--;
        break;

    case 0xF8:          // RM
        if (state->f.s) // if sign flag set. Perform RET which pops stack into program counter
        {
            state->pc = (state->mem[state->sp + 1] << 8) | state->mem[state->sp]; // Jump to the return address
            state->sp += 2;
        }
        break;

    case 0xF9: // SPHL
        hl = (state->h << 8) | state->l;
        state->sp = hl;
        break;

    case 0xFA: // JM adr (jump if minus)
        if (state->f.s)
        {
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xFB: // EI - Special: enables interrupts
        state->int_enable = 1;
        break;

    case 0xFC: // CM adr (CALL if minus)
        if (state->f.s)
        {
            result = state->pc + 2;
            state->mem[state->sp - 1] = (result >> 8) & 0xFF;
            state->mem[state->sp - 2] = (result & 0xFF);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
            state->pc--;
        }
        else
        {
            state->pc += 2;
        }
        break;

    case 0xFD: // NOP
        break;

    case 0xFE:                // CPI D8 - Subtract data from accumulator, set flags with result
        lowerdec = opcode[1]; // Store value for subtraction
        state->f.z = state->a == lowerdec;
        state->f.cy = lowerdec > state->a;
        result = state->a - lowerdec;
        state->f.ac = (result & 0x0F) == 0x0F;
        state->f.s = 0x80 == (result & 0x80);
        state->f.p = Parity(result & 0xFF);
        state->pc += 1;
        break;

    case 0xFF:                                     // RST 7
        result = state->pc;                        // Store the address of the next instruction on the stack
        state->mem[state->sp - 1] = (result >> 8); // store the higher bits of the addr in the higher stack addr
        state->mem[state->sp - 2] = result & 0xff; // store the lower bits of the address in the lower stack addr
        state->sp -= 2;                            // stack grows downward
        state->pc = 0x0038;                        // sets pc to 8 multiplied by the number associated with RST (8*7)
        state->pc--;
        break;
    }
    (state->pc)++;

    // print out the processor state, flags and registers after execution
    /*printf("\tC=%d,P=%d,S=%d,Z=%d\n", state->f.cy, state->f.p,
           state->f.s, state->f.z);
    printf("\tA $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n",
           state->a, state->b, state->c, state->d,
           state->e, state->h, state->l, state->sp);*/
    // Potentially short term return, expect failed cases will return 1?
    return 0;
}