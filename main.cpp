
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <sifrpc.h>

static char padBuffer[256] __attribute__((aligned(64)));

int main() {
    // Inicializar mando
    SifInitRpc(0);
    padInit(0);
    padPortOpen(0, 0, padBuffer);

    // Inicializar gráficos
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
    const u64 floorColor = GS_SETREG_RGBAQ(65, 55, 65, 0x80, 0);
    const u64 wallColor = GS_SETREG_RGBAQ(105, 85, 85, 0x80, 0);
    const u64 playerColor = GS_SETREG_RGBAQ(220, 205, 190, 0x80, 0);

    float cx = gs->Width / 2.0f;
    float cy = gs->Height / 2.0f;

    float left = cx - 145;
    float right = cx + 145;
    float top = cy - 110;
    float bottom = cy + 110;

    // Posición inicial de Isaac
    float playerX = cx;
    float playerY = cy;

    const float speed = 2.2f;

    struct padButtonStatus buttons;

    while (1) {
        float dx = 0.0f;
        float dy = 0.0f;

        // Leer mando cuando está listo
        int state = padGetState(0, 0);

        if (state == PAD_STATE_STABLE ||
            state == PAD_STATE_FINDCTP1) {

            if (padRead(0, 0, &buttons) != 0) {
                unsigned short pressed =
                    (unsigned short)(~buttons.btns);

                // Cruceta
                if (pressed & PAD_LEFT)  dx = -1.0f;
                if (pressed & PAD_RIGHT) dx =  1.0f;
                if (pressed & PAD_UP)    dy = -1.0f;
                if (pressed & PAD_DOWN)  dy =  1.0f;

                // Joystick izquierdo
                if (buttons.ljoy_h < 100) dx = -1.0f;
                if (buttons.ljoy_h > 155) dx =  1.0f;
                if (buttons.ljoy_v < 100) dy = -1.0f;
                if (buttons.ljoy_v > 155) dy =  1.0f;
            }
        }

        // Evitar que la diagonal sea más rápida
        if (dx != 0.0f && dy != 0.0f) {
            dx *= 0.7071f;
            dy *= 0.7071f;
        }

        // Movimiento
        playerX += dx * speed;
        playerY += dy * speed;

        // Colisiones con las paredes
        if (playerX < left + 22) playerX = left + 22;
        if (playerX > right - 22) playerX = right - 22;
        if (playerY < top + 24) playerY = top + 24;
        if (playerY > bottom - 24) playerY = bottom - 24;

        // Dibujar habitación
        gsKit_clear(gs, background);

        gsKit_prim_sprite(
            gs, left, top, right, bottom, 1, floorColor
        );

        gsKit_prim_sprite(
            gs, left, top, right, top + 12, 2, wallColor
        );

        gsKit_prim_sprite(
            gs, left, bottom - 12, right, bottom, 2, wallColor
        );

        gsKit_prim_sprite(
            gs, left, top, left + 12, bottom, 2, wallColor
        );

        gsKit_prim_sprite(
            gs, right - 12, top, right, bottom, 2, wallColor
        );

        // Dibujar Isaac en su nueva posición
        gsKit_prim_sprite(
            gs,
            playerX - 10,
            playerY - 12,
            playerX + 10,
            playerY + 12,
            3,
            playerColor
        );

        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }

    return 0;
}
