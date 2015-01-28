AR=ar
ARFLAGS=rcs
CC=gcc
CFLAGS=-Wall -fpermissive -Wwrite-strings
CPP=g++
CPPFLAGS=-Wall -fpermissive -Wwrite-strings
LDFLAGS=

# source files
SOURCES=main.cpp chip8instruction.cpp
HEADERS=chip8instruction.h
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
