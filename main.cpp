#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>

#include "game.h"

static unsigned char padBuffer[256] __attribute__((aligned(64)));

int main() {
    GSGLOBAL* gs = gsKit_init_global();

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

    SifInitRpc(0);
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    padInit(0);

    int padReady = padPortOpen(0, 0, padBuffer);
    struct padButtonStatus buttons;

    game_init(gs->Width, gs->Height, gs);

    while (1) {
        int moveX = 0;
        int moveY = 0;
        int fireX = 0;
        int fireY = 0;

        if (padReady != 0) {
            int state = padGetState(0, 0);

            if (state == PAD_STATE_STABLE && padRead(0, 0, &buttons) != 0) {
                unsigned short pressed = (unsigned short)(~buttons.btns);

                // Left analog stick (0-255, center around 128).
                // A generous dead zone avoids drift on older controllers.
                if (buttons.ljoy_h < 96) moveX = -1;
                if (buttons.ljoy_h > 160) moveX = 1;
                if (buttons.ljoy_v < 96) moveY = -1;
                if (buttons.ljoy_v > 160) moveY = 1;

                // D-pad remains available and takes priority.
                if (pressed & PAD_LEFT)  moveX = -1;
                if (pressed & PAD_RIGHT) moveX = 1;
                if (pressed & PAD_UP)    moveY = -1;
                if (pressed & PAD_DOWN)  moveY = 1;

                // Shooting: face buttons.
                if (pressed & PAD_SQUARE)   fireX = -1;
                if (pressed & PAD_CIRCLE)   fireX = 1;
                if (pressed & PAD_TRIANGLE) fireY = -1;
                if (pressed & PAD_CROSS)    fireY = 1;
            }
        }

        game_update(moveX, moveY, fireX, fireY);
        game_draw(gs);
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }

    return 0;
}
