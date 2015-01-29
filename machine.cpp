#include "machine.h"
#include <string.h> //memset
#include <stdlib.h> //rand
#include <unistd.h> //sleep

// font set
uint8_t chip8_fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Machine::Machine() :
   i(0),
   pc(0),
   sp(0),
   kill(false)
{
   // init memories
   memset(memory, MEMORY_SIZE, sizeof(uint8_t));
   memset(v, GENERAL_REGS, sizeof(uint8_t));
   memset(stack, STACK_SIZE, sizeof(uint16_t));
   
   // init fonts
   memcpy(memory, chip8_fontset, 80*sizeof(uint8_t));
}

Machine::~Machine()
{
}

void Machine::execute(uint8_t* program, int length)
{
   // set program counter / stack pointer
   pc = START_ADDRESS;
   sp = 0;
   
   // copy the program into memory
   memcpy(&(memory[pc]), program, length);
   
   while((!kill) && ((pc+1)<MEMORY_SIZE) && (pc != 0))
   {
      // get instruction pointed at by pc
      uint16_t opcode = (memory[pc]<<8) | memory[pc+1];
      
      printf("pc=0x%04X opcode=0x%04X\n",pc, opcode);
      //sleep(1);
      
      // decode instruction
      switch(opcode&0xF000)
      {
         case 0x0000: //
            switch(opcode&0x00F0)
            {
               case 0x00C0: // scroll screen down N
                  pc += 2;
                  break;
                  
               case 0x00E0:
                  switch(opcode&0x00FF)
                  {
                     case 0x00E0: // clear screen
                        break;
                     case 0x00EE: // ret
                        printf("return sp=%x sp[0]=%x sp[1]=%x\n", sp, stack[0], stack[1]);
                        sp--;
                        pc = stack[sp];
                        break;
                        
                     default:
                        printf("0x%04X : bad opcode\n", opcode);
                        break;
                  }
                  pc += 2;
                  break;
               
               case 0x00F0:
                  switch(opcode&0x00FF)
                  {
                     case 0x00FA: // compatibility mode
                     case 0x00FB: // scroll screen right
                     case 0x00FC: // scroll screen left
                     case 0x00FE: // low resolution
                     case 0x00FF: // high resolution
                     default:
                        break;
                  }
                  pc += 2;
                  break;
                  
               default: // assume 0NNN : jmp NNN
                  pc = opcode&0x0fff;
                  break;
            }
            break;

         case 0x1000: // 1NNN : jmp NNN
            pc = opcode&0x0fff;
            break;
            
         case 0x2000: // 2NNN : jsr NNN
            printf("push %x onto stack @ %i\n", pc, sp);
            stack[sp++] = pc;  // push current onto stack
            pc = opcode&0x0fff; // set pc
            break;
  
         case 0x3000: // 3XNN
            if(v[(opcode>>8)&0x000f] == (opcode&0x00ff))
               pc+=2;
            pc+=2;
            //printf("skip.eq V%i,0x%x\n", (opcode>>8)&0x000f, opcode&0x00ff);
            break;
            
         case 0x4000: // 4XNN
            if(v[(opcode>>8)&0x000f] != (opcode&0x00ff))
               pc+=2;
            pc+=2;
            //printf("skip.ne V%i,0x%x\n", (opcode>>8)&0x000f, opcode&0x00ff);
            break;

         case 0x5000: // 5XY0
            if(v[(opcode>>8)&0x000f] == v[(opcode>>4)&0x000f])
               pc+=2;
            pc+=2;
            //printf("skip.eq V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
            break;

         case 0x6000: // 6XNN
            v[(opcode>>8)&0x000f] = (opcode&0x00ff);
            pc+=2;
            //printf("mov V%i,0x%x\n", (opcode>>8)&0x000f, opcode&0x00ff);
            break;

         case 0x7000: // 7XNN
            v[(opcode>>8)&0x000f] += (opcode&0x00ff);
            pc+=2;
            //printf("add V%i,0x%x\n", opcode&0x00ff, (opcode>>8)&0x000f);
            break;
            
         case 0x8000:
            switch(opcode&0x000F)
            {
               case 0x0000: // //8XY0
                  v[(opcode>>8)&0x000f] = v[(opcode>>4)&0x000f];
                  //printf("mov V%i,V%i\n", x, y);
                  break;
                  
               case 0x0001: // or
                  v[(opcode>>8)&0x000f] |= v[(opcode>>4)&0x000f];
                  //printf("or V%i,V%i\n", x, y);
                  break;
                  
               case 0x0002: // and
                  v[(opcode>>8)&0x000f] &= v[(opcode>>4)&0x000f];
                  //printf("and V%i,V%i\n", x, y);
                  break;
                  
               case 0x0003: // xor
                  v[(opcode>>8)&0x000f] ^= v[(opcode>>4)&0x000f];
                  //printf("xor V%i,V%i\n", x, y);
                  break;
                  
               case 0x0004: // //8XY4
                  if( (v[(opcode>>8)&0x000f] + v[(opcode>>4)&0x000f]) > 0xFF )
                     v[0xF]=1;
                  else
                     v[0xF]=0;
                  v[(opcode>>8)&0x000f] += v[(opcode>>4)&0x000f];
                  //printf("add.c V%i,V%i\n", x, y);
                  break;
                  
               case 0x0005: // // 8XY5
                  if( (v[(opcode>>8)&0x000f] - v[(opcode>>4)&0x000f]) < 0 )
                     v[0xF]=1;
                  else
                     v[0xF]=0;
                  v[(opcode>>8)&0x000f] -= v[(opcode>>4)&0x000f];
                  //printf("sub.b V%i,V%i\n", x, y);
                  break;
                  
               case 0x0006: // 8XY6
                  v[0xF] = v[(opcode>>8)&0x000F]&0x1;
                  v[(opcode>>8)&0x000F] >>= 1;
                  //printf("shr\n");
                  break;
                  
               case 0x0007:
                  //printf("rsb V%i,V%i\n", x, y);
                  break;
                  
               case 0x000E: // shift left
                  v[0xF] = (v[(opcode>>8)&0x000F]>>0xf)&0x1;
                  v[(opcode>>8)&0x000F] <<= 1;
                  //printf("shl\n");
                  break;
                  
               default:
                  printf("0x%04X : bad opcode\n", opcode);
                  break;
            }
            pc+=2;
            break;

         case 0x9000: // 9XY0
            if(v[(opcode>>8)&0x000f] != v[(opcode>>4)&0x000f])
               pc+=2;
            pc+=2;
            //printf("skip.ne V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
            break;

         case 0xA000: // ANNN
            i = opcode&0x0fff;
            pc+=2;
            //printf("mov I,0x%x\n", opcode&0x0fff);
            break;

         case 0xB000: // BNNN
            pc = (opcode&0x0fff) + v[0];
            //printf("jmp 0x%x+V0\n", opcode&0x0fff);
            break;

         case 0xC000: // CXNN
            v[(opcode>>8)&0x000f] = rand()&(opcode&0x00ff);
            pc+=2;
            //printf("rand V%i,rnd&0x%x\n", (opcode>>8)&0x000f, opcode&0x00ff);
            break;

         case 0xD000: // DXYN
            pc+=2;
            //uint8_t x = (opcode>>8)&0x000f;
            //uint8_t y = (opcode>>4)&0x000f;
            //uint8_t n = opcode&0x000f;
            //printf("sprite V%i,V%i,%i\n", x, y, n);
            break;

         case 0xE000:
            switch(opcode&0x00FF)
            {
               case 0x009E:
                  //if()
                     //pc+=2;
                  //printf("skip.press V%i\n", x);
                  break;
                  
               case 0x00A1:
                  //if()
                     //pc+=2;
                  //printf("skip.npress V%i\n", x);
                  break;

               default:
                  printf("0x%04X : bad opcode\n", opcode);
                  break;
            }
            pc+=2;
            break;

         case 0xF000:
            switch(opcode&0x00FF)
            {
               case 0x0007:
                  //printf("gdelay V%i\n", x);
                  break;
                  
               case 0x000A:
                  //printf("key V%i\n", x);
                  break;
                  
               case 0x0015:
                  //printf("sdelay V%i\n", x);
                  break;
                  
               case 0x0018:
                  //printf("ssound V%i\n", x);
                  break;
                  
               case 0x001E:
                  i += v[(opcode>>8)&0x000F];
                  //printf("add I,V%i\n", x);
                  break;
                  
               case 0x0029:
                  //printf("font V%i\n", x);
                  break;
                  
               case 0x0030:
                  //printf("font V%i\n", x);
                  break;
                  
               case 0x0033:
                  //printf("bcd I,V%i\n", x);
                  break;
                  
               case 0x0055: // FX55 - stores V0 to VX in memory starting at address I
                  for(int indx=0; indx<((opcode>>8)&0x000F); indx++)
                     memory[i+indx] = v[indx];
                  //printf("store [I],V0-VX\n");
                  break;
                  
               case 0x0065:
                  //printf("load V0-VX,[I]\n");
                  break;

               default:
                  printf("0x%04X : bad opcode\n", opcode);
                  break;
            }
            pc+=2;
            break;
////////////////////////////////////////////

         default:
            printf("0x%04X : bad opcode\n", opcode);
            pc+=2;
            break;
      }
      
      // 
   }
}