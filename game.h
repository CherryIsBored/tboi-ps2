#ifndef GAME_H
#define GAME_H

#include <gsKit.h>

struct Player {
    float x, y, speed;
    float vx, vy;
    int health;
};

struct Tear {
    float x, y, vx, vy;
    int life;
    int active;
};

struct Enemy {
    float x, y, speed;
    int hp;
    int active;
    int flash;
};

struct Pickup {
    float x, y;
    int type; // 0 heart, 1 coin, 2 key, 3 bomb
    int active;
};

void game_init(int width, int height, GSGLOBAL* gs);
void game_update(int moveX, int moveY, int fireX, int fireY, int startPressed);
void game_draw(GSGLOBAL* gs);

#endif
