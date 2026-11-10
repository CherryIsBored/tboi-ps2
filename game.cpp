
#include "game.h"

static Player isaac;
static Tear tears[16];

void game_init() {
    isaac.x = 320.0f;
    isaac.y = 240.0f;
    isaac.speed = 2.5f;
    isaac.health = 6;

    for (int i = 0; i < 16; i++) {
        tears[i].active = 0;
    }
}

void game_update() {
    // Movimiento y disparos se implementan a continuación.
}

void game_draw() {
    // Renderizado del juego se implementa a continuación.
}
