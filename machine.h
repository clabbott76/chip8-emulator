#ifndef MACHINE_H
#define MACHINE_H

#include <stdio.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include "SDL/SDL.h"

/** 
 * Hardware specs were taken from :
 * http://en.wikipedia.org/wiki/CHIP-8 ***
 * http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
 */

#define MEMORY_SIZE 0x1000
#define GENERAL_REGS 16
#define STACK_SIZE   16

// display layout
// -------------------
// |(0,0)     (63, 0)|
// |                 |
// |(0,31)    (63,31)|
// -------------------
#define SCREEN_WIDTH  64
#define SCREEN_HEIGHT 32

// starting address of program, emulator occupies memory from 0x0-0x1FF
#define START_ADDRESS 0x200

class Machine
{
public:
   Machine();
   ~Machine();
   
   void disassemble(uint8_t *program,
                    int     length);
   
   /**
    * Executes a program.
    * 
    * @param[in] program: The pointer to the program code
    * @param[in] length:  The length of the program in bytes
    */
   void execute(uint8_t *program,
                int     length);
   
   /**
    * decodes an instruction. depending on flags will either decode to readable
    * string or emulate the instruction.
    *
    * @param[in] opcode:  The instruction
    * @param[in] emulate: Flag so instruction is emulated
    * @param[in] decode:  Flag so instruction is decoded
    */
   bool decode(uint16_t opcode,
               bool emulate,
               bool decode);
   
private:
   void updateTimers();
   void initGraphics();
   void drawGraphics();
   void cleanupGraphics();
   void pollInputs();
   
   // memory
   uint8_t memory[MEMORY_SIZE];
   
   // registers (16 general) (1 address aka index)
   uint8_t v[GENERAL_REGS];
   uint16_t I;
   
   // fixed stack size, allows call depth of 16
   uint16_t stack[STACK_SIZE];
   
   // screen buffer
   uint8_t screen[SCREEN_WIDTH*SCREEN_HEIGHT];
   
   // flag that indicates we need to draw the screen
   bool drawFlag;
   
   // keys
   uint8_t keys[16];
   
   // program counter
   uint16_t pc;
   
   // stack pointer
   uint8_t sp;
   
   // flag used to kill the execute loop
   bool kill;
   
   // timer counters
   uint8_t delayTimer;
   uint8_t soundTimer;

#ifdef BUILD_X11
   // X11 window stuff
   Display *d;
   Window window;
   XEvent e;
   int s;
#endif

#ifdef BUILD_SDL
   SDL_Surface* screenSurface;
   SDL_Surface* backbuff;
#endif
};

#endif //MACHINE_H
