
#include <stdio.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>

static unsigned char padBuffer[256] __attribute__((aligned(64)));

int main() {
    GSGLOBAL* gs = gsKit_init_global();
    GSFONTM* font = gsKit_init_fontm();

    dmaKit_init(
        D_CTRL_RELE_OFF,
        D_CTRL_MFD_OFF,
        D_CTRL_STS_UNSPEC,
        D_CTRL_STD_OFF,
        D_CTRL_RCYC_8,
        1 << DMA_CHANNEL_GIF
    );

    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);
    gsKit_fontm_upload(gs, font);

    u64 bg = GS_SETREG_RGBAQ(15, 15, 25, 0x80, 0);
    u64 white = GS_SETREG_RGBAQ(255, 255, 255, 0x80, 0);
    u64 green = GS_SETREG_RGBAQ(50, 230, 100, 0x80, 0);
    u64 red = GS_SETREG_RGBAQ(240, 60, 60, 0x80, 0);

    SifInitRpc(0);

    int sio = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    int pad = SifLoadModule("rom0:PADMAN", 0, NULL);

    int initResult = padInit(0);
    int openResult = padPortOpen(0, 0, padBuffer);

    struct padButtonStatus buttons;

    int state = 0;
    int readResult = 0;
    unsigned short pressed = 0;

    char line[100];

    while (1) {
        state = padGetState(0, 0);
        readResult = 0;
        pressed = 0;

        if (state == PAD_STATE_STABLE) {
            readResult = padRead(0, 0, &buttons);

            if (readResult != 0) {
                pressed = (unsigned short)(~buttons.btns);
            }
        }

        gsKit_clear(gs, bg);

        gsKit_fontm_print_scaled(
            gs, font, 30, 30, 2, 0.8f, white,
            "ISAAC PS2 - PAD TEST"
        );

        sprintf(line, "SIO2MAN: %d", sio);
        gsKit_fontm_print_scaled(gs, font, 30, 90, 2, 0.7f, white, line);

        sprintf(line, "PADMAN: %d", pad);
        gsKit_fontm_print_scaled(gs, font, 30, 125, 2, 0.7f, white, line);

        sprintf(line, "padInit: %d", initResult);
        gsKit_fontm_print_scaled(gs, font, 30, 160, 2, 0.7f, white, line);

        sprintf(line, "padPortOpen: %d", openResult);
        gsKit_fontm_print_scaled(gs, font, 30, 195, 2, 0.7f, white, line);

        sprintf(line, "PAD STATE: %d", state);
        gsKit_fontm_print_scaled(
            gs, font, 30, 245, 2, 0.8f,
            state == PAD_STATE_STABLE ? green : red, line
        );

        sprintf(line, "PAD READ: %d", readResult);
        gsKit_fontm_print_scaled(gs, font, 30, 285, 2, 0.8f, white, line);

        sprintf(line, "BUTTON MASK: %04X", pressed);
        gsKit_fontm_print_scaled(gs, font, 30, 330, 2, 0.8f, white, line);

        gsKit_fontm_print_scaled(
            gs, font, 30, 390, 2, 0.7f, white,
            "PRESS ANY BUTTON"
        );

        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }

    return 0;
}
