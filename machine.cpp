#include "machine.h"
#include <string.h> //memset()
#include <stdlib.h> //rand()
#include <unistd.h> //sleep()
#include <time.h> //time() difftime()

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
   I(0),
   drawFlag(false),
   pc(0),
   sp(0),
   kill(false)
{
   // init memories
   memset(memory, 0, MEMORY_SIZE*sizeof(uint8_t));
   memset(v, 0, GENERAL_REGS*sizeof(uint8_t));
   memset(stack, 0, STACK_SIZE*sizeof(uint16_t));
   
   // init timers
   delayTimer=0;
   soundTimer=0;
   
   // init fonts
   for(int i=0; i<80; i++)
      memory[i] = chip8_fontset[i];
   
   // init graphics
   for(int i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++)
         screen[i]=0;
   
   // init keys
   for(int i=0; i<16; i++)
      keys[i]=0;
   
   // get the graphics started
   initGraphics();

   // initialize random seed
   srand(time(NULL));
}

Machine::~Machine()
{
}

void Machine::disassemble(uint8_t* program, int length)
{
   int badcodes = 0;

   printf("\n");
   printf("addr  op  note\n");
   printf("---- ---- ---------------\n");
   
   uint16_t instr = 0;
   for(int i=0; i<length; i+=2)
   {
      instr = (program[i]<<8) | program[i+1];
      printf("%04x %04x ", i+0x200, instr);
      
      if( !decode(instr, false, true) )
      {
         //printf("unknown/bad opcode\n");
         ++badcodes;
      }
   }
   
   printf("\ntotal instructions %i\n", length/2);
   printf("found %i unknown/bad instructions\n", badcodes);
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
      // wait for user input
      //fgetc(stdin);
      //for(int b=0; b<16; b++) printf("V[%i]=x%02X ", b, v[b]);
      //printf("\n");
      //printf("I=0x%x\n", I);

      // sleep to slow down
      usleep(500);

      // *** fetch ***
      uint16_t opcode = (memory[pc]<<8) | memory[pc+1];
      
      // *** decode ***
      decode(opcode, true, false);
      
      // *** update timers ***
      updateTimers();
      
      // *** update screen ***
      if(drawFlag)
      {
         drawGraphics();
         drawFlag = false;
      }

      // *** process inputs ***
      pollInputs();
   } // while
   
   // let's cleanup
   cleanupGraphics();
}

bool Machine::decode(uint16_t opcode,
                     bool emulate,
                     bool decode)
{
   bool valid = true; // assume true for now
   
   //printf("opcode 0x%04x\n", opcode);
   
   switch(opcode&0xF000)
   {
      //****************//
      case 0x0000:
      {
         switch(opcode&0x00FF)
         {
            case 0x00E0: // 00E0    Clears the screen.
            {
               if(emulate)
               {
                  for(int i=0; i<SCREEN_HEIGHT*SCREEN_WIDTH; i++)
                  {
                     screen[i]=0;
                     drawFlag = true;
                  }
               }
               if(decode)
               {
                  printf("cls\n");
               }
            }
            break;

            case 0x00EE: // 00EE   Returns from a subroutine.
            {
               if(emulate)
               {
                  sp--;
                  pc = stack[sp];
               }
               if(decode)
               {
                  printf("rtn\n");
               }
            }
            break;
               
            default:
               printf("unknown opcode\n");
               valid = false;
               break;
         }
         pc += 2; // either commands will increment the pc
      }
      break;

      //****************//
      case 0x1000: // 1NNN    Jumps to address NNN.
      {
         if(emulate)
         {
            pc = opcode&0x0FFF;
         }
         if(decode)
         {
            printf("jmp 0x%x\n", opcode&0x0FFF);
         }
      }
      break;
      
      //****************//
      case 0x2000: // 2NNN    Calls subroutine at NNN.
      {
         if(emulate)
         {
            stack[sp++] = pc;  // push current onto stack
            pc = opcode&0x0FFF; // set pc
         }
         if(decode)
         {
            printf("jsr 0x%x\n", opcode&0x0FFF);
         }
      }
      break;

      //****************//
      case 0x3000: // 3XNN    Skips the next instruction if VX equals NN.
      {
         if(emulate)
         {
            if(v[(opcode>>8)&0x000F] == (opcode&0x00FF))
               pc+=2;
            pc+=2;
         }
         if(decode)
         {
            printf("skip.eq V%i,0x%x\n", (opcode>>8)&0x000F, opcode&0x00FF);
         }
      }
      break;
      
      //****************//
      case 0x4000: // 4XNN    Skips the next instruction if VX doesn't equal NN.
      {
         if(emulate)
         {
            if(v[(opcode>>8)&0x000F] != (opcode&0x00FF))
               pc+=2;
            pc+=2;
         }
         if(decode)
         {
            printf("skip.ne V%i,0x%x\n", (opcode>>8)&0x000F, opcode&0x00FF);
         }
      }
      break;

      //****************//
      case 0x5000: // 5XY0    Skips the next instruction if VX equals VY.
      {
         if(emulate)
         {
            if(v[(opcode>>8)&0x000f] == v[(opcode>>4)&0x000f])
               pc+=2;
            pc+=2;
         }
         if(decode)
         {
            printf("skip.eq V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
         }
      }
      break;

      //****************//
      case 0x6000: // 6XNN    Sets VX to NN.
      {
         if(emulate)
         {
            v[(opcode>>8)&0x000f] = (opcode&0x00ff);
            pc+=2;
         }
         if(decode)
         {
            printf("mov V%i,0x%x\n", (opcode>>8)&0x000f, opcode&0x00ff);
         }
      }
      break;

      //****************//
      case 0x7000: // 7XNN    Adds NN to VX.
      {
         if(emulate)
         {
            v[(opcode>>8)&0x000f] += (opcode&0x00ff);
            pc+=2;
         }
         if(decode)
         {
            printf("add V%i,0x%x\n", (opcode>>8)&0x000f, opcode&0x00ff);
         }
      }
      break;
       
      //****************//
      case 0x8000:
      {
         switch(opcode&0x000F)
         {
            case 0x0000: // 8XY0    Sets VX to the value of VY.
            {
               if(emulate)
               {
                  v[(opcode>>8)&0x000f] = v[(opcode>>4)&0x000f];
               }
               if(decode)
               {
                  printf("mov V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
               }
            }
            break;
               
            case 0x0001: // 8XY1    Sets VX to VX or VY.
            {
               if(emulate)
               {
                  v[(opcode>>8)&0x000f] |= v[(opcode>>4)&0x000f];
               }
               if(decode)
               {
                  printf("or V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
               }
            }
            break;
               
            case 0x0002: // 8XY2    Sets VX to VX and VY.
            {
               if(emulate)
               {
                  v[(opcode>>8)&0x000f] &= v[(opcode>>4)&0x000f];
               }
               if(decode)
               {
                  printf("and V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
               }
            }
            break;
               
            case 0x0003: // 8XY3    Sets VX to VX xor VY.
            {
               if(emulate)
               {
                  v[(opcode>>8)&0x000f] ^= v[(opcode>>4)&0x000f];
               }
               if(decode)
               {
                  printf("xor V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
               }
            }
            break;
               
            case 0x0004: // 8XY4    Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
            {
               if(emulate)
               {
                  if( (v[(opcode>>8)&0x000f] + v[(opcode>>4)&0x000f]) > 0xFF )
                     v[0xF]=1;
                  else
                     v[0xF]=0;
                  v[(opcode>>8)&0x000f] += v[(opcode>>4)&0x000f];
               }
               if(decode)
               {
                  printf("add.c V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
               }
            }
            break;
               
            case 0x0005: // 8XY5    VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
            {
               if(emulate)
               {
                  if( (v[(opcode>>8)&0x000f] - v[(opcode>>4)&0x000f]) < 0 )
                     v[0xF]=1;
                  else
                     v[0xF]=0;
                  v[(opcode>>8)&0x000f] -= v[(opcode>>4)&0x000f];
               }
               if(decode)
               {
                  printf("sub.b V%i,V%i\n", (opcode>>8)&0x000F, (opcode>>4)&0x000f);
               }
            }
            break;
               
            case 0x0006: // 8XY6    Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.
            {
               if(emulate)
               {
                  v[0xF] = v[(opcode>>8)&0x000F]&0x1;
                  v[(opcode>>8)&0x000F] >>= 1;
               }
               if(decode)
               {
                  printf("shr V%i\n", (opcode>>8)&0x000F);
               }
               
               
            }
            break;
               
            case 0x0007: // 8XY7    Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
            {
               if(emulate)
               {
                  if(v[(opcode>>4)&0x000F] > (0xFF - v[(opcode&0x0F00)]))
                     v[0xF] = 1; // set carry
                  else
                     v[0xF] = 0;
                  v[(opcode>>8)&0x000F] = v[(opcode>>4)&0x000F] - v[(opcode>>8)&0x000F];
               }
               if(decode)
               {
                  printf("rsb V%i,V%i\n", (opcode>>8)&0x000F, (opcode>>4)&0x000F);
               }
               
            }
            break;

            case 0x000E: // 8XYE    Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.
            {
               if(emulate)
               {
                  v[0xF] = (v[(opcode>>8)&0x000F]>>0xf)&0x1;
                  v[(opcode>>8)&0x000F] <<= 1;
               }
               if(decode)
               {
                  printf("shl V%i\n", (opcode>>8)&0x000F);
               }
            }
            break;
               
            default:
               printf("unknown opcode\n");
               valid = false;
               break;
         }
         pc+=2;
      }
      break;

      //****************
      case 0x9000: // 9XY0    Skips the next instruction if VX doesn't equal VY.
      {
         if(emulate)
         {
            if(v[(opcode>>8)&0x000f] != v[(opcode>>4)&0x000f])
               pc+=2;
            pc+=2;
         }
         if(decode)
         {
            printf("skip.ne V%i,V%i\n", (opcode>>8)&0x000f, (opcode>>4)&0x000f);
         }
      }
      break;

      //****************
      case 0xA000: // ANNN    Sets I to the address NNN.
      {
         if(emulate)
         {
            I = opcode&0x0fff;
            pc+=2;
         }
         if(decode)
         {
            printf("mov I,0x%x\n", opcode&0x0fff);
         }
      }
      break;

      //****************
      case 0xB000: // BNNN    Jumps to the address NNN plus V0.
      {
         if(emulate)
         {
            pc = (opcode&0x0fff) + v[0];
         }
         if(decode)
         {
            printf("jmp 0x%x+V0\n", opcode&0x0fff);
         }
      }
      break;

      //****************
      case 0xC000: // CXNN  Sets VX to a random number and NN.
      {
         if(emulate)
         {
            v[(opcode>>8)&0x000f] = (rand()%255)&(opcode&0x00ff);
            pc+=2;
         }
         if(decode)
         {
            printf("rand V%i,rnd&0x%x\n", (opcode>>8)&0x000f, opcode&0x00ff);
         }
      }
      break;

      //****************
      case 0xD000:   // DXYN    Sprites stored in memory at location in index register (I), maximum 8bits wide. 
      {              //         Wraps around the screen. If when drawn, clears a pixel, register VF is set to 1 
                     //         otherwise it is zero. All drawing is XOR drawing (i.e. it toggles the screen pixels)
         uint8_t x = v[(opcode>>8)&0x000F];
         uint8_t y = v[(opcode>>4)&0x000F];
         uint8_t n = opcode&0x000F;
         if(emulate)
         {
            uint8_t pixel;

            v[0xF] = 0;
            for (int yline = 0; yline < n; yline++)
            {
               pixel = memory[I + yline];
               for(int xline = 0; xline < 8; xline++)
               {
                  if((pixel & (0x80 >> xline)) != 0)
                  {
                     if(screen[(x + xline + ((y + yline) * 64))] == 1)
                     {
                        v[0xF] = 1;
                     }
                     screen[x + xline + ((y + yline) * 64)] ^= 1;
                  }
               }
            }
            drawFlag = true;
            pc+=2;
         }
         if(decode)
         {
            printf("sprite V%i,V%i,%i\n", (opcode>>8)&0x000F, (opcode>>4)&0x000F, opcode&0x000F);
         }
      }
      break;

      //****************
      case 0xE000:
      {
         switch(opcode&0x00FF)
         {
            case 0x009E: // EX9E    Skips the next instruction if the key stored in VX is pressed.
            {
               if(emulate)
               {
                  if(keys[v[(opcode>>8)&0xF]] > 0)
                     pc+=2;
               }
               if(decode)
               {
                  printf("skip.press V%i\n", (opcode>>8)&0xF);
               }
            }
            break;

            case 0x00A1: // EXA1    Skips the next instruction if the key stored in VX isn't pressed.
            {
               if(emulate)
               {
                  if(keys[v[(opcode>>8)&0xF]] == 0)
                     pc+=2;
               }
               if(decode)
               {
                  printf("skip.npress V%i\n", (opcode>>8)&0xF);
               }
            }
            break;

            default:
               printf("unknown opcode\n");
               valid = false;
               break;
         }
         pc+=2;
      }
      break;

      //****************
      case 0xF000:
      {
         switch(opcode&0x00FF)
         {
            case 0x0007: // FX07    Sets VX to the value of the delay timer.
            {
               if(emulate)
               {
                  v[(opcode>>8)&0xF] = delayTimer;
               }
               if(decode)
               {
                  printf("gdelay V%i\n", (opcode>>8)&0xF);
               }
            }
            break;
               
            case 0x000A: // FX0A   A key press is awaited, and then stored in VX.
            {
               if(emulate)
               {
                  int waitKey=0;
                  for(waitKey=0; waitKey<16; waitKey++)
                  {
                     if(keys[waitKey] > 0)
                     {
                        v[(opcode>>8)&0xF] = waitKey;
                        break;
                     }
                  }
                  if(waitKey==16)
                     pc -= 2; // do not increment the pc reg
               }
               if(decode)
               {
                  printf("key V%i\n", (opcode>>8)&0xF);
               }
            }
            break;
               
            case 0x0015: // FX15    Sets the delay timer to VX.
            {
               if(emulate)
               {
                  delayTimer = (opcode>>8)&0xF;
               }
               if(decode)
               {
                  printf("sdelay V%i\n", (opcode>>8)&0x000F);
               }
            }
            break;
               
            case 0x0018: // FX18    Sets the sound timer to VX.
            {
               if(emulate)
               {
                  soundTimer = (opcode>>8)&0xF;
               }
               if(decode)
               {
                  printf("ssound V%i\n", (opcode>>8)&0x000F);
               }
            }
            break;
               
            case 0x001E: // FX1E    Adds VX to I.
            {
               if(emulate)
               {
                  I += v[(opcode>>8)&0x000F];
               }
               if(decode)
               {
                  printf("add I,V%i\n", (opcode>>8)&0x000F);
               }
            }
            break;
               
            case 0x0029: // FX29    Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
            {
               if(emulate)
               {
                  I = v[(opcode>>8)&0xF] * 5;
               }
               if(decode)
               {
                  printf("font I,V%i\n", (opcode>>8)&0x000F);
               }
            }
            break;

            case 0x0033: // FX33    Stores the Binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the 
            {            //         middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take the decimal representation 
                         //         of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.)
               if(emulate)
               {
                  memory[I+2] =  v[(opcode>>8)&0xF] % 10; // least significant
                  memory[I+1] = (v[(opcode>>8)&0xF] / 10) % 10;
                  memory[I]   =  v[(opcode>>8)&0xF] / 100;
               }
               if(decode)
               {
                  printf("bcd I,V%i\n", (opcode>>8)&0x000F);
               }
            }
            break;
               
            case 0x0055: // FX55 - stores V0 to VX in memory starting at address I
            {
               if(emulate)
               {
                  for(int indx=0; indx<=((opcode>>8)&0x000F); indx++)
                     memory[I+indx] = v[indx];
               }
               if(decode)
               {
                  printf("store [I],V0-V%i\n", (opcode>>8)&0x000F);
               }
            }
            break;
               
            case 0x0065: // FX65  Fills V0 to VX with values from memory starting at address I
            {
               if(emulate)
               {
                  for(int indx=0; indx<=((opcode>>8)&0x000F); indx++)
                     v[indx] = memory[I+indx];
               }
               if(decode)
               {
                  printf("load V0-V%i,[I]\n", (opcode>>8)&0x000F);
               }
            }
            break;

            default:
               printf("unknown opcode\n");
               valid = false;
               break;
         }
         pc+=2;
      }
      break;

      default:
         printf("unknown opcode\n");
         if(emulate)
         {
            pc+=2;
         }
         valid = false;
         break;
         
   } // switch
      
   return valid;
}

void Machine::updateTimers()
{
   static int c = 0;

   // the timers are only updated every 25 instructions. This seems to look ok
   if(++c == 25)
   {
      // *** update delay timer ***
      if(delayTimer > 0)
         --delayTimer;

      // *** update sound timer ***
      if(soundTimer > 0)
         --soundTimer;

      c = 0;
   }
}

void Machine::initGraphics()
{
#ifdef BUILD_X11
   // setup display borrowed from
   // http://rosettacode.org/wiki/Window_creation/X11
   d = XOpenDisplay(NULL);
   if (d == NULL)
   {
      fprintf(stderr, "Cannot open display\n");
      exit(1);
   }

   s = DefaultScreen(d);
   window = XCreateSimpleWindow(d,                 // display
                                RootWindow(d, s),  // parent
                                0,                 // x
                                0,                 // y
                                SCREEN_WIDTH*10,   // width
                                SCREEN_HEIGHT*10,  // height
                                1,                 // border width
                                BlackPixel(d, s),  // border
                                WhitePixel(d, s)); // background

   XSelectInput(d, window, ExposureMask | KeyPressMask);
   XMapWindow(d, window);
   XFlush(d);
#endif

#ifdef BUILD_SDL

   //Start SDL
   SDL_Init( SDL_INIT_EVERYTHING );

   //Set up screen
   backbuff = NULL;
   screenSurface = NULL;
   screenSurface = SDL_SetVideoMode( SCREEN_WIDTH*10, SCREEN_HEIGHT*10, 32, SDL_SWSURFACE );
#endif
}

void Machine::drawGraphics()
{
#ifdef BUILD_X11
   for(int x=0; x<SCREEN_WIDTH; x++)
   {
      for(int y=0; y<SCREEN_HEIGHT; y++)
      {
         if(screen[(y*SCREEN_WIDTH)+x] == 1)
            XFillRectangle(d,               // display
                           window,          // window
                           DefaultGC(d, s), // GC ???
                           x*10,            // x
                           y*10,            // y
                           10,              // width
                           10);             // height
         else
            XClearArea(d,      // display
                       window, // window
                       x*10,   // x
                       y*10,   // y
                       10,     // width
                       10,     // height
                       false); //
      }
   }
   XFlush(d);
#endif

#ifdef BUILD_SDL
      for(int x=0; x<SCREEN_WIDTH; x++)
      {
         for(int y=0; y<SCREEN_HEIGHT; y++)
         {
            SDL_Rect rect;
            rect.x = x*10;
            rect.y = y*10;
            rect.w = 10;
            rect.h = 10;
            if(screen[(y*SCREEN_WIDTH)+x] == 1)
               SDL_FillRect(screenSurface, &rect, SDL_MapRGB(screenSurface->format, 255, 255, 255));
            else
               SDL_FillRect(screenSurface, &rect, SDL_MapRGB(screenSurface->format, 0, 0, 0));
         }
      }
      SDL_Flip(screenSurface);
#endif
}

void Machine::cleanupGraphics()
{
#ifdef BUILD_X11
   // cleanup X11
   XCloseDisplay(d);
#endif

#ifdef BUILD_SDL
   //Quit SDL
   SDL_Quit();
#endif
}

void Machine::pollInputs()
{
#ifdef BUILD_X11
   for(int i=0; i<16; i++)
   {
      if(keys[i] > 0)
         keys[i]-=1;
   }
   while(XEventsQueued(d,QueuedAlready))
   //while(XPending(d))
   {
      uint8_t keystate = 0;
      XNextEvent(d, &e);
      if(e.type == KeyPress)
         keystate = 100;
      else if (e.type == KeyRelease)
         keystate = 0;

     //printf("KeyPress: keycode %u state %u\n", e.xkey.keycode, e.xkey.state);
     switch(e.xkey.keycode)
     {
        case 10: //"1"
        case 11: //"2"
        case 12: //"3"
        case 13: //"4"
           keys[e.xkey.keycode-10] = keystate;
           break;
        case 24: //"q"
        case 25: //"w"
        case 26: //"e"
        case 27: //"r"
           keys[e.xkey.keycode-20] = keystate;
           break;
        case 38: //"a"
        case 39: //"s"
        case 40: //"d"
        case 41: //"f"
           keys[e.xkey.keycode-30] = keystate;
           break;
        case 52: //"z"
        case 53: //"x"
        case 54: //"c"
        case 55: //"v"
           keys[e.xkey.keycode-40] = keystate;
           break;
        case 9: //"esc"
           kill=true;
           break;
     }
   } // while(pending)
#endif

#ifdef BUILD_SDL
   //Handle events on queue
   SDL_Event e;
   while( SDL_PollEvent( &e ) != 0 )
   {
      //User requests quit
      if( e.type == SDL_QUIT )
      {
         kill = true;
      }
      //User presses a key
      else if( (e.type == SDL_KEYDOWN) || (e.type == SDL_KEYUP) )
      {
         uint8_t action = 0; // key up
         if(e.type == SDL_KEYDOWN)
         {
            action = 1;
         }

         //Select surfaces based on key press
         switch( e.key.keysym.sym )
         {
         case SDLK_1:
            keys[0] = action;
            break;
         case SDLK_2:
            keys[1] = action;;
            break;
         case SDLK_3:
            keys[2] = action;
            break;
         case SDLK_4:
            keys[3] = action;
            break;
         case SDLK_q:
            keys[4] = action;
            break;
         case SDLK_w:
            keys[5] = action;
            break;
         case SDLK_e:
            keys[6] = action;
            break;
         case SDLK_r:
            keys[7] = action;
            break;
         case SDLK_a:
            keys[8] = action;
            break;
         case SDLK_s:
            keys[9] = action;
            break;
         case SDLK_d:
            keys[10] = action;
            break;
         case SDLK_f:
            keys[11] = action;
            break;
         case SDLK_z:
            keys[12] = action;
            break;
         case SDLK_x:
            keys[13] = action;
            break;
         case SDLK_c:
            keys[14] = action;
            break;
         case SDLK_v:
            keys[15] = action;
            break;
         case SDLK_ESCAPE:
            kill = true;
            break;
         default:
            break;
         }
      }
   }
#endif
}
