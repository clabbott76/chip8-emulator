#uncomment to build with SDL
GFXLIB=BUILD_SDL
# uncomment to build with X11
#GFXLIB=BUILD_X11

AR=ar
ARFLAGS=rcs
CC=gcc
CFLAGS=-g -Wall -fpermissive -Wwrite-strings -D$(GFXLIB)
CPP=g++
CPPFLAGS=-g -Wall -fpermissive -Wwrite-strings -D$(GFXLIB)
LDFLAGS=-lX11 -lSDL

# source files
SOURCES=main.cpp machine.cpp
HEADERS=machine.h
# object files
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=c8emul

# default rule
all : $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS) $(HEADERS)
	$(CPP) $(OBJECTS) $(LDFLAGS) -o $@

# rule to make any .o from a .cpp file
%.o : %.cpp
	$(CPP) -c $(CPPFLAGS) $<

# rule to make any .o from a .c file
%.o : %.c
	$(CC) -c $(CFLAGS) $<

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
