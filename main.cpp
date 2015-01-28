#include <stdio.h>
#include <stdint.h> //uint8_t
#include <stdlib.h> //malloc

void printHelp(char* app)
{
   printf("Usage: %s\n", app);
   printf("\n");
}

void hexdump(uint8_t* binary, int length)
{
   int address = 0;
 
   printf("HEXDUMP %i bytes\n", length);
   
   if((binary == NULL) || (length <= 0))
      return;
   
   for(int i=0; i<length; i++)
   {
      if((i%16)==0)
      {
         printf("%07x ", address);
         address += 0x10;
      }
      
      printf("%02x ", binary[i]);
      
      // if end of line OR last byte
       if((((i+1)%16)==0) || ((i+1)==length))
       {
          printf("\n");
       }
   }
}

void disassemble(uint8_t* binary, int length)
{
   int badcodes = 0;

   printf("\n");
   printf("addr  op  note\n");
   printf("---- ---- ---------------\n");
   
   uint16_t instr = 0;
   for(int i=0; i<length; i+=2)
   {
      instr = (binary[i+1]<<8) | (binary[i]);
      printf("%04x %04x ", i, instr);
      
      // shift down upper nibble
      uint8_t subopcode = 0; // used for opcodes with similar upper nibbles
      uint8_t opcode = instr >> 12;
      switch(opcode)
      {
         case 0:
            {
               if(instr == 0x00ee) // return from subroutine
                  printf("return\n");
               else if(instr == 0x0e0) // clear screen
                  printf("clear\n");
               else if((instr&0x0000) == 0)
                  printf("call program @ NNN\n");
            }
            break;
         case 1:
            { // 1NNN
               uint16_t addr = instr&0x0fff;
               printf("jump to 0x%x\n", addr);
            }
            break;
         case 2:
            { // 2NNN
               uint16_t addr = instr&0x0fff;
               printf("call subroutine @ 0x%x\n", addr);
            }
            break;
         case 3:
            { // 3XNN
               uint8_t v = (instr>>8)&0x000f;
               uint8_t nn = instr&0x00ff;
               printf("skip next instr if V%i == 0x%x\n", v, nn);
            }
            break;
         case 4:
            { // 4XNN
               uint8_t v = (instr>>8)&0x000f;
               uint8_t nn = instr&0x00ff;
               printf("skip next instr if V%i != 0x%x\n", v, nn);
            }
            break;
         case 5:
            { // 5XY0
               uint8_t x = (instr>>8)&0x000f;
               uint8_t y = (instr>>4)&0x000f;
               printf("skip next instr if V%i == V%i\n", x, y);
            }
            break;
         case 6:
            { // 6XNN
               uint8_t x = (instr>>8)&0x000f;
               uint8_t nn = instr&0x00ff;
               printf("sets V%i to 0x%x\n", x, nn);
            }
            break;
         case 7:
            { // 7XNN
               uint8_t x = (instr>>8)&0x000f;
               uint8_t nn = instr&0x00ff;
               printf("adds 0x%x to V%i\n", nn, x);
            }
            break;
         case 8:
            subopcode = (uint8_t) instr & 0x000f;
            switch(subopcode)
            {
               case 0x0: // VX=VY
                  printf("VX=VY\n");
                  break;
               case 0x1: // or
                  printf("or\n");
                  break;
               case 0x2: // and
                  printf("and\n");
                  break;
               case 0x3: // xor
                  printf("xor\n");
                  break;
               case 0x4: // VX=VY+VX
                  printf("VX=VY+VX\n");
                  break;
               case 0x5: // VX=VY-VX
                  printf("VX=VY-VX\n");
                  break;
               case 0x6: // shift right
                  printf("shift right\n");
                  break;
               case 0x7: // VX=VY-VX
                  printf("VX=VY-VX\n");
                  break;
               case 0xe: // shift left
                  printf("shift left\n");
                  break;
               case 0xf: // ???
                  printf("FIX THIS CRAP 0x800f\n");
                  break;
               default:
                  printf("8xxx bad subopcode\n");
                  badcodes++;
                  break;
            }
            break;
         case 9:
            { // 9XY0
               uint8_t x = (instr>>8)&0x000f;
               uint8_t y = (instr>>4)&0x000f;
               printf("skip next instr if V%i != V%i\n", x, y);
            }
            break;
         case 0xa:
            { // ANNN
               uint16_t addr = instr&0x0fff;
               printf("set I to 0x%x\n", addr);
            }
            break;
         case 0xb:
            { // BNNN
               uint16_t addr = instr&0x0fff;
               printf("jump to address 0x%x + V0\n", addr);
            }
            break;
         case 0xc:
            { // CXNN
               uint8_t x = (instr>>8)&0x000f;
               uint8_t nn = instr&0x00ff;
               printf("set V%i to rnd# and 0x%x\n", x, nn);
            }
            break;
         case 0xd:
            printf("sprintes\n");
            break;
         case 0xe:
            subopcode = instr & 0x00ff;
            switch(subopcode)
            {
               case 0x9e:
                  printf("skip if key in VX is pressed\n");
                  break;
               case 0xa1:
                  printf("skip if key in VX isn't pressed\n");
                  break;
               case 0xa2:
                  printf("FIX THIS CRAP 0xEXA2\n");
                  break;
               default:
                  printf("bad E--- opcode\n");
                  badcodes++;
                  break;
            }
            break;
         case 0xf:
            subopcode = instr & 0x00ff;
            switch(subopcode)
            {
               case 0x07:
                  printf("VX=delay timer\n");
                  break;
               case 0x0a:
                  printf("VX=key press\n");
                  break;
               case 0x15:
                  printf("delay timer=VX\n");
                  break;
               case 0x18:
                  printf("sound timer=VX\n");
                  break;
               case 0x1e:
                  printf("I=VX+I\n");
                  break;
               case 0x29:
                  printf("I=location of sprite\n");
                  break;
               case 0x33:
                  printf("store BCD of VX to @ of I\n");
                  break;
               case 0x55:
                  printf("store V0 to VX\n");
                  break;
               case 0x65:
                  printf("fill V0 to VX\n");
                  break;
               case 0x69:
               case 0xa2:
               case 0x7d:
               case 0x7b:
               case 0x60:
                  printf("FIX THIS CRAP\n");
                  break;
               default:
                  printf("0xf000 ???\n");
                  badcodes++;
                  break;
            }
            break;
         default:
            printf("?\n");
            badcodes++;
            break;
      }
   }
   
   printf("\ntotal instructions %i\n", length/2);
   printf("found %i bad instructions\n", badcodes);
}

int main(int argc, char* argv[])
{  
   if(argc<2)
   {
      printHelp(argv[0]);
      return 0;
   }
   
   FILE* f = (FILE*) fopen(argv[1], "r");
   if(f != NULL) // if pointer is valid
   {
      // how big is file
      fseek(f, 0, SEEK_END);
      int fsize = ftell(f);
      fseek(f, 0, SEEK_SET);
      // read in whole file
      uint8_t* binary = (uint8_t*) malloc(fsize+1);
      fread(binary, fsize, sizeof(uint8_t), f);
      fclose(f); // close file
      
      // hexdump
      hexdump(binary, fsize);
      
      // disassemble
      disassemble(binary, fsize);
      
      free(binary); // cleanup memory
   }
   
   return 0;
}