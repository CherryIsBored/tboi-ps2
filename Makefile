
EE_BIN = isaac-ps2.elf
EE_OBJS = main.o game.o

EE_INCS += -I$(GSKIT)/ee/gs/include
EE_INCS += -I$(GSKIT)/ee/dma/include

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

EE_LIBS += -L$(GSKIT)/lib -lgskit -ldmakit -lpad

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
