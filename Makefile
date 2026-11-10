
EE_BIN = isaac-ps2.elf
EE_OBJS = main.o
EE_LIBS =

EE_CXX = g++
EE_CC = gcc

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
