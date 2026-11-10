
#include <gsKit.h>
#include <dmaKit.h>

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

    const u64 background = GS_SETREG_RGBAQ(20, 17, 24, 0x80, 0);
    const u64 floor      = GS_SETREG_RGBAQ(65, 55, 65, 0x80, 0);
    const u64 wall       = GS_SETREG_RGBAQ(105, 85, 85, 0x80, 0);
    const u64 player     = GS_SETREG_RGBAQ(220, 205, 190, 0x80, 0);

    float cx = gs->Width / 2.0f;
    float cy = gs->Height / 2.0f;

    float left   = cx - 145;
    float right  = cx + 145;
    float top    = cy - 110;
    float bottom = cy + 110;

    while (1) {
        gsKit_clear(gs, background);

        // Piso de la habitación
        gsKit_prim_sprite(
            gs, left, top, right, bottom, 1, floor
        );

        // Paredes
        gsKit_prim_sprite(
            gs, left, top, right, top + 12, 2, wall
        );

        gsKit_prim_sprite(
            gs, left, bottom - 12, right, bottom, 2, wall
        );

        gsKit_prim_sprite(
            gs, left, top, left + 12, bottom, 2, wall
        );

        gsKit_prim_sprite(
            gs, right - 12, top, right, bottom, 2, wall
        );

        // Isaac
        gsKit_prim_sprite(
            gs, cx - 10, cy - 12,
            cx + 10, cy + 12, 3, player
        );

        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }

    return 0;
}
