
EE_BIN = isaac-ps2.elf
EE_OBJS = main.o

EE_CXX = $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-g++
EE_CC = $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-gcc

EE_CFLAGS = -O2
EE_CXXFLAGS = -O2 -std=c++17

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
