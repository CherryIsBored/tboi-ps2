
EE_BIN = isaac-ps2.elf
EE_OBJS = main.o game.o

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

EE_LIBS += -lgskit -ldmakit -lpad

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
