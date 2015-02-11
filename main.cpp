#include <stdio.h>
#include <stdint.h> //uint8_t
#include <stdlib.h> //malloc
#include <string.h>
#include "machine.h"

void printHelp(char* app)
{
   printf("Usage: %s [-?hde] FILE\n", app);
   printf(" ?\tDisplay this help menu\n");
   printf(" h\tPerform hex dump\n");
   printf(" d\tPerform disassembly\n");
   printf(" e\tPerform emulation\n");
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
       if( (((i+1)%16)==0) || ((i+1)==length) )
       {
          printf("\n");
       }
   }
}

int main(int argc, char* argv[])
{
   bool dump=false;
   bool diss=false;
   bool emulate=false;
   
   if(argc<3)
   {
      printHelp(argv[0]);
      return 0;
   }
   
   // validate options
   if(argv[1][0] == '-')
   {
      if( strstr(argv[1], "?") != NULL )
      {
         printHelp(argv[0]);
         return 0;
      }
         
      if( strstr(argv[1], "h") != NULL )
         dump=true;
      
      if( strstr(argv[1], "d") != NULL )
         diss=true;
      
      if( strstr(argv[1], "e") != NULL )
         emulate=true;
   }
   else
   {
      printf("invalid option\n");
      printHelp(argv[0]);
      return -1;
   }
   
   FILE* f = (FILE*) fopen(argv[2], "r");
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
      if(dump)
         hexdump(binary, fsize);
      
      Machine mach;
      // disassemble
      if(diss)
         mach.disassemble(binary, fsize);
      
      // emulate
      if(emulate)
         mach.execute(binary, fsize);
      
      // cleanup memory
      free(binary);
   }
   
   return 0;
}