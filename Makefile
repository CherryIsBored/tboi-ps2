EE_BIN = isaac-ps2.elf
EE_OBJS = main.o

EE_INCS += -I$(PS2DEV)/gsKit/include
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib
EE_LIBS += -lgskit -ldmakit

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

EE_LIBS += -lgskit -ldmakit

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
