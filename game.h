
#ifndef GAME_H
#define GAME_H

#include <gsKit.h>

struct Player {
    float x;
    float y;
    float speed;
    int health;
};

struct Tear {
    float x;
    float y;
    float vx;
    float vy;
    int active;
};

void game_init(int width, int height);
void game_update(int moveX, int moveY, int fireX, int fireY);
void game_draw(GSGLOBAL* gs);

#endif
