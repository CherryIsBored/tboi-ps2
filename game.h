#ifndef GAME_H
#define GAME_H

#include <gsKit.h>

struct Player {
    float x, y, speed;
    int health;
};

struct Tear {
    float x, y, vx, vy;
    int active;
};

struct Enemy {
    float x, y, speed;
    int hp;
    int active;
    int flash;
};

void game_init(int width, int height, GSGLOBAL* gs);
void game_update(int moveX, int moveY, int fireX, int fireY);
void game_draw(GSGLOBAL* gs);

#endif
