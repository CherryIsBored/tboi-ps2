
#ifndef GAME_H
#define GAME_H

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

void game_init();
void game_update();
void game_draw();

#endif
