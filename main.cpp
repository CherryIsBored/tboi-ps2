#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>

#include "game.h"

static unsigned char padBuffer[256] __attribute__((aligned(64)));

int main() {
    GSGLOBAL* gs = gsKit_init_global();

    // The GS has only 4 MB of VRAM. gsKit defaults to CT24 + double
    // buffering + a 32-bit Z buffer, which consumes almost all of it before
    // the game's textures are uploaded. Use a 16-bit framebuffer and disable
    // the depth buffer (the game is strictly 2D and draws in explicit order).
    gs->PSM = GS_PSM_CT16;
    gs->PSMZ = GS_PSMZ_16;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    // Textured sprites use 0..128 GS alpha; enable the GS source-over blend
    // so transparent atlas pixels reveal the room instead of a black rectangle.
    gs->PrimAlphaEnable = GS_SETTING_ON;

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
    int padModeRequested = 0;

    game_init(gs->Width, gs->Height, gs);

    while (1) {
        int moveX = 0;
        int moveY = 0;
        int fireX = 0;
        int fireY = 0;
        int startPressed = 0;

        if (padReady != 0) {
            int state = padGetState(0, 0);

            if (state == PAD_STATE_STABLE) {
                // Explicitly enable DualShock analog mode once the pad is ready.
                if (!padModeRequested) {
                    int modes = padInfoMode(0, 0, PAD_MODETABLE, -1);
                    int hasDualShock = 0;
                    for (int i = 0; i < modes; i++) {
                        if (padInfoMode(0, 0, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK) {
                            hasDualShock = 1;
                            break;
                        }
                    }
                    if (hasDualShock) {
                        padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
                    }
                    padModeRequested = 1;
                }

                if (padRead(0, 0, &buttons) != 0) {
                    unsigned short pressed = (unsigned short)(~buttons.btns);

                    // Read analog axes only after the controller reports DualShock mode.
                    if (padInfoMode(0, 0, PAD_MODECURID, 0) == PAD_TYPE_DUALSHOCK) {
                        if (buttons.ljoy_h < 96) moveX = -1;
                        if (buttons.ljoy_h > 160) moveX = 1;
                        if (buttons.ljoy_v < 96) moveY = -1;
                        if (buttons.ljoy_v > 160) moveY = 1;
                    }

                    // D-pad overrides analog input and always remains usable.
                    if (pressed & PAD_LEFT)  moveX = -1;
                    if (pressed & PAD_RIGHT) moveX = 1;
                    if (pressed & PAD_UP)    moveY = -1;
                    if (pressed & PAD_DOWN)  moveY = 1;

                    if (pressed & PAD_START) startPressed = 1;
                    if (pressed & PAD_SQUARE)   fireX = -1;
                    if (pressed & PAD_CIRCLE)   fireX = 1;
                    if (pressed & PAD_TRIANGLE) fireY = -1;
                    if (pressed & PAD_CROSS)    fireY = 1;
                }
            }
        }

        game_update(moveX, moveY, fireX, fireY, startPressed);
        game_draw(gs);
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }

    return 0;
}
