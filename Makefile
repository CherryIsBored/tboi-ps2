
EE_BIN = ISAAC-PS2.ELF
EE_OBJS = main.o game.o

EE_INCS += -I$(GSKIT)/include
EE_INCS += -I$(PS2SDK)/ports/include

EE_LDFLAGS += -L$(GSKIT)/lib
EE_LDFLAGS += -L$(PS2SDK)/ports/lib

EE_LIBS += -lgskit -ldmakit -lpad

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
