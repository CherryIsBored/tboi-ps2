
#include "game.h"

static Player isaac;
static Tear tears[16];

static float roomLeft;
static float roomRight;
static float roomTop;
static float roomBottom;

static int tearCooldown = 0;

void game_init(int width, int height) {
    roomLeft = width / 2.0f - 145.0f;
    roomRight = width / 2.0f + 145.0f;
    roomTop = height / 2.0f - 110.0f;
    roomBottom = height / 2.0f + 110.0f;

    isaac.x = width / 2.0f;
    isaac.y = height / 2.0f;
    isaac.speed = 2.5f;
    isaac.health = 6;

    tearCooldown = 0;

    for (int i = 0; i < 16; i++) {
        tears[i].active = 0;
    }
}

void game_update(int moveX, int moveY, int fireX, int fireY) {
    float dx = (float)moveX;
    float dy = (float)moveY;

    if (dx != 0 && dy != 0) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }

    isaac.x += dx * isaac.speed;
    isaac.y += dy * isaac.speed;

    if (isaac.x < roomLeft + 20) isaac.x = roomLeft + 20;
    if (isaac.x > roomRight - 20) isaac.x = roomRight - 20;
    if (isaac.y < roomTop + 20) isaac.y = roomTop + 20;
    if (isaac.y > roomBottom - 20) isaac.y = roomBottom - 20;

    if (tearCooldown > 0) {
        tearCooldown--;
    }

    if ((fireX != 0 || fireY != 0) && tearCooldown == 0) {
        for (int i = 0; i < 16; i++) {
            if (!tears[i].active) {
                tears[i].x = isaac.x;
                tears[i].y = isaac.y;
                tears[i].vx = fireX * 6.0f;
                tears[i].vy = fireY * 6.0f;
                tears[i].active = 1;
                tearCooldown = 12;
                break;
            }
        }
    }

    for (int i = 0; i < 16; i++) {
        if (!tears[i].active) continue;

        tears[i].x += tears[i].vx;
        tears[i].y += tears[i].vy;

        if (tears[i].x < roomLeft + 12 ||
            tears[i].x > roomRight - 12 ||
            tears[i].y < roomTop + 12 ||
            tears[i].y > roomBottom - 12) {
            tears[i].active = 0;
        }
    }
}

void game_draw(GSGLOBAL* gs) {
    u64 background = GS_SETREG_RGBAQ(20, 17, 24, 0x80, 0);
    u64 floor = GS_SETREG_RGBAQ(75, 65, 65, 0x80, 0);
    u64 wall = GS_SETREG_RGBAQ(105, 85, 85, 0x80, 0);
    u64 playerColor = GS_SETREG_RGBAQ(220, 205, 190, 0x80, 0);
    u64 tearColor = GS_SETREG_RGBAQ(100, 190, 255, 0x80, 0);

    gsKit_clear(gs, background);

    gsKit_prim_sprite(
        gs, roomLeft, roomTop,
        roomRight, roomBottom, 1, floor
    );

    // Room walls
    gsKit_prim_sprite(
        gs, roomLeft, roomTop,
        roomRight, roomTop + 12, 2, wall
    );

    gsKit_prim_sprite(
        gs, roomLeft, roomBottom - 12,
        roomRight, roomBottom, 2, wall
    );

    gsKit_prim_sprite(
        gs, roomLeft, roomTop,
        roomLeft + 12, roomBottom, 2, wall
    );

    gsKit_prim_sprite(
        gs, roomRight - 12, roomTop,
        roomRight, roomBottom, 2, wall
    );

    // Isaac
    gsKit_prim_sprite(
        gs, isaac.x - 10, isaac.y - 12,
        isaac.x + 10, isaac.y + 12, 3, playerColor
    );

    // Tears
    for (int i = 0; i < 16; i++) {
        if (!tears[i].active) continue;

        gsKit_prim_sprite(
            gs,
            tears[i].x - 4,
            tears[i].y - 4,
            tears[i].x + 4,
            tears[i].y + 4,
            4,
            tearColor
        );
    }
}
