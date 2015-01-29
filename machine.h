#ifndef MACHINE_H
#define MACHINE_H

#include <stdio.h>
#include <stdint.h>

//*** hardware specs were taken from http://en.wikipedia.org/wiki/CHIP-8 ***
#define MEMORY_SIZE 0x1000
#define GENERAL_REGS 16
#define STACK_SIZE   16

// starting address of program, emulator occupies memory from 0x0-0x1FF
#define START_ADDRESS 0x200

class Machine
{
public:
   Machine();
   ~Machine();
   
   /**
    * Executes a program.
    * 
    * @param[in] program: The pointer to the program code
    * @param[in] length:  The length of the program in bytes
    */
   void execute(uint8_t* program, int length);
   
private:
   // memory
   uint8_t memory[MEMORY_SIZE];
   
   // registers (16 general) (1 address aka index)
   uint8_t v[GENERAL_REGS];
   uint16_t i;
   
   // fixed stack size, allows call depth of 16
   uint16_t stack[STACK_SIZE];
   
   // program counter
   uint16_t pc;
   
   // stack pointer
   uint8_t sp;
   
   // flag used to kill the execute loop
   bool kill;
};

#endif //MACHINE_H
