#include "chip8instruction.h"

Chip8Instruction::Chip8Instruction()
{
}

Chip8Instruction::~Chip8Instruction()
{
}

void Chip8Instruction::decode(uint8_t* binary, int length)
{
   if((binary == NULL) || (length <= 0))
      return;
}