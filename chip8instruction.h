#ifndef CHIP8INSTRUCTION_H
#define CHIP8INSTRUCTION_H

#include <stdio.h>
#include <stdint.h>

class Chip8Instruction
{
public:
   Chip8Instruction();
   ~Chip8Instruction();
   
   void decode(uint8_t* binary, int length);
};

#endif //CHIP8INSTRUCTION_H