#include "game.h"
#include "sprite_data.h"
#include "pickup_data.h"

static Player isaac;
static Tear tears[16];
static Enemy enemies[6];
static Pickup pickups[8];
static int coins = 0, keys = 0, bombs = 0;
static GSTEXTURE isaacTextures[4][10];
static GSTEXTURE pickupTextures[4];

static float roomLeft, roomRight, roomTop, roomBottom;
static int tearCooldown = 0;
static int damageCooldown = 0;
static int facing = 0; // 0 down, 1 up, 2 left, 3 right
static int walkFrame = 0;
static int walkTick = 0;
static int roomNumber = 1;
static int roomCleared = 0;
static int enemiesRemaining = 0;
static int hitFlash = 0;

static float absf(float v) { return v < 0.0f ? -v : v; }

static void drop_pickup(float x, float y, int seed) {
    for (int i = 0; i < 8; i++) {
        if (pickups[i].active) continue;
        pickups[i].x = x; pickups[i].y = y;
        int roll = (seed * 7 + roomNumber * 3 + i * 5) % 10;
        pickups[i].type = (roll < 5) ? 1 : ((roll < 7) ? 0 : ((roll < 9) ? 2 : 3));
        pickups[i].active = 1;
        break;
    }
}

static void spawn_room() {
    for (int i = 0; i < 6; i++) enemies[i].active = 0;
    int count = 2 + (roomNumber % 3);
    enemiesRemaining = count;
    roomCleared = 0;
    for (int i = 0; i < count; i++) {
        enemies[i].x = roomLeft + 65.0f + (float)((i * 83 + roomNumber * 29) % 210);
        enemies[i].y = roomTop + 55.0f + (float)((i * 61 + roomNumber * 47) % 150);
        enemies[i].hp = 2 + (roomNumber > 3 ? 1 : 0);
        enemies[i].speed = 0.42f + (float)(roomNumber % 3) * 0.08f;
        enemies[i].active = 1;
        enemies[i].flash = 0;
    }
}

void game_init(int width, int height, GSGLOBAL* gs) {
    roomLeft = width / 2.0f - 145.0f;
    roomRight = width / 2.0f + 145.0f;
    roomTop = height / 2.0f - 110.0f;
    roomBottom = height / 2.0f + 110.0f;

    isaac.x = width / 2.0f;
    isaac.y = height / 2.0f;
    isaac.speed = 2.5f;
    isaac.health = 6;
    tearCooldown = damageCooldown = 0;
    coins = keys = bombs = 0;
    for (int i = 0; i < 8; i++) pickups[i].active = 0;
    facing = walkFrame = walkTick = 0;
    roomNumber = 1;
    hitFlash = 0;

    for (int i = 0; i < 16; i++) tears[i].active = 0;

    for (int d = 0; d < 4; d++) {
        for (int f = 0; f < 10; f++) {
            GSTEXTURE* texture = &isaacTextures[d][f];
            texture->Width = 32;
            texture->Height = 48;
            texture->PSM = GS_PSM_CT32;
            texture->ClutPSM = GS_PSM_CT32;
            texture->Mem = (u32*)isaac_pixels[d][f];
            texture->Clut = NULL;
            texture->VramClut = 0;
            texture->Filter = GS_FILTER_NEAREST;
            texture->Delayed = GS_SETTING_OFF;
            texture->Vram = gsKit_vram_alloc(gs, gsKit_texture_size(32, 48, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
            gsKit_texture_upload(gs, texture);
        }
    }
    for (int p = 0; p < 4; p++) {
        GSTEXTURE* texture = &pickupTextures[p];
        texture->Width = 16; texture->Height = 16;
        texture->PSM = GS_PSM_CT32; texture->ClutPSM = GS_PSM_CT32;
        texture->Mem = (u32*)pickup_pixels[p]; texture->Clut = NULL;
        texture->VramClut = 0; texture->Filter = GS_FILTER_NEAREST;
        texture->Delayed = GS_SETTING_OFF;
        texture->Vram = gsKit_vram_alloc(gs, gsKit_texture_size(16, 16, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
        gsKit_texture_upload(gs, texture);
    }
    spawn_room();
}

void game_update(int moveX, int moveY, int fireX, int fireY) {
    float dx = (float)moveX, dy = (float)moveY;
    if (dx != 0 && dy != 0) { dx *= 0.7071f; dy *= 0.7071f; }

    if (moveX < 0) facing = 2;
    else if (moveX > 0) facing = 3;
    else if (moveY < 0) facing = 1;
    else if (moveY > 0) facing = 0;

    if (moveX || moveY) {
        if (++walkTick >= 2) { walkTick = 0; walkFrame = (walkFrame + 1) % 10; }
    } else { walkFrame = 0; walkTick = 0; }

    isaac.x += dx * isaac.speed;
    isaac.y += dy * isaac.speed;
    if (isaac.x < roomLeft + 20) isaac.x = roomLeft + 20;
    if (isaac.x > roomRight - 20) isaac.x = roomRight - 20;
    if (isaac.y < roomTop + 20) isaac.y = roomTop + 20;
    if (isaac.y > roomBottom - 20) isaac.y = roomBottom - 20;

    if (tearCooldown > 0) tearCooldown--;
    if ((fireX || fireY) && tearCooldown == 0) {
        for (int i = 0; i < 16; i++) if (!tears[i].active) {
            tears[i].x = isaac.x; tears[i].y = isaac.y;
            tears[i].vx = fireX * 6.0f; tears[i].vy = fireY * 6.0f;
            tears[i].active = 1; tearCooldown = 12; break;
        }
    }

    for (int i = 0; i < 16; i++) {
        if (!tears[i].active) continue;
        tears[i].x += tears[i].vx; tears[i].y += tears[i].vy;
        if (tears[i].x < roomLeft + 12 || tears[i].x > roomRight - 12 ||
            tears[i].y < roomTop + 12 || tears[i].y > roomBottom - 12) tears[i].active = 0;
        for (int e = 0; e < 6 && tears[i].active; e++) {
            if (!enemies[e].active) continue;
            if (absf(tears[i].x - enemies[e].x) < 15 && absf(tears[i].y - enemies[e].y) < 15) {
                enemies[e].hp--;
                enemies[e].flash = 8;
                tears[i].active = 0;
                if (enemies[e].hp <= 0) {
                    drop_pickup(enemies[e].x, enemies[e].y, e + roomNumber);
                    enemies[e].active = 0;
                    enemiesRemaining--;
                    if (enemiesRemaining <= 0) roomCleared = 1;
                }
            }
        }
    }

    for (int p = 0; p < 8; p++) {
        if (!pickups[p].active) continue;
        if (absf(isaac.x - pickups[p].x) < 18 && absf(isaac.y - pickups[p].y) < 18) {
            if (pickups[p].type == 0) { if (isaac.health < 6) isaac.health++; }
            else if (pickups[p].type == 1) coins++;
            else if (pickups[p].type == 2) keys++;
            else if (pickups[p].type == 3) bombs++;
            pickups[p].active = 0;
        }
    }

    for (int e = 0; e < 6; e++) {
        if (!enemies[e].active) continue;
        float ex = isaac.x - enemies[e].x;
        float ey = isaac.y - enemies[e].y;
        float len = absf(ex) + absf(ey);
        if (len > 0.1f) {
            enemies[e].x += (ex / len) * enemies[e].speed;
            enemies[e].y += (ey / len) * enemies[e].speed;
        }
        if (enemies[e].flash > 0) enemies[e].flash--;
        if (absf(isaac.x - enemies[e].x) < 20 && absf(isaac.y - enemies[e].y) < 20 && damageCooldown == 0) {
            if (isaac.health > 0) isaac.health--;
            damageCooldown = 75;
            hitFlash = 12;
            isaac.x += (isaac.x < enemies[e].x ? -12.0f : 12.0f);
            isaac.y += (isaac.y < enemies[e].y ? -12.0f : 12.0f);
        }
    }
    if (damageCooldown > 0) damageCooldown--;
    if (hitFlash > 0) hitFlash--;

    // Once the room is clear, touching a doorway advances to the next room.
    if (roomCleared) {
        int atDoor = (isaac.x < roomLeft + 25 || isaac.x > roomRight - 25 ||
                      isaac.y < roomTop + 25 || isaac.y > roomBottom - 25);
        if (atDoor) {
            roomNumber++;
            isaac.x = (roomLeft + roomRight) * 0.5f;
            isaac.y = (roomTop + roomBottom) * 0.5f;
            spawn_room();
        }
    }
}

void game_draw(GSGLOBAL* gs) {
    u64 background = GS_SETREG_RGBAQ(20, 17, 24, 0x80, 0);
    u64 floor = GS_SETREG_RGBAQ(75, 65, 65, 0x80, 0);
    u64 wall = GS_SETREG_RGBAQ(105, 85, 85, 0x80, 0);
    u64 tearColor = GS_SETREG_RGBAQ(100, 190, 255, 0x80, 0);
    gsKit_clear(gs, background);
    gsKit_prim_sprite(gs, roomLeft, roomTop, roomRight, roomBottom, 1, floor);
    gsKit_prim_sprite(gs, roomLeft, roomTop, roomRight, roomTop + 12, 2, wall);
    gsKit_prim_sprite(gs, roomLeft, roomBottom - 12, roomRight, roomBottom, 2, wall);
    gsKit_prim_sprite(gs, roomLeft, roomTop, roomLeft + 12, roomBottom, 2, wall);
    gsKit_prim_sprite(gs, roomRight - 12, roomTop, roomRight, roomBottom, 2, wall);

    // Door openings become visible after every enemy is defeated.
    u64 doorColor = roomCleared ? GS_SETREG_RGBAQ(80, 170, 90, 0x80, 0) : GS_SETREG_RGBAQ(45, 35, 38, 0x80, 0);
    gsKit_prim_sprite(gs, (roomLeft + roomRight) * .5f - 15, roomTop, (roomLeft + roomRight) * .5f + 15, roomTop + 12, 3, doorColor);
    gsKit_prim_sprite(gs, (roomLeft + roomRight) * .5f - 15, roomBottom - 12, (roomLeft + roomRight) * .5f + 15, roomBottom, 3, doorColor);
    gsKit_prim_sprite(gs, roomLeft, (roomTop + roomBottom) * .5f - 15, roomLeft + 12, (roomTop + roomBottom) * .5f + 15, 3, doorColor);
    gsKit_prim_sprite(gs, roomRight - 12, (roomTop + roomBottom) * .5f - 15, roomRight, (roomTop + roomBottom) * .5f + 15, 3, doorColor);

    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ATEST_OFF);
    for (int p = 0; p < 8; p++) if (pickups[p].active) {
        gsKit_prim_sprite_texture(gs, &pickupTextures[pickups[p].type],
            pickups[p].x - 8, pickups[p].y - 8, 0, 0,
            pickups[p].x + 8, pickups[p].y + 8, 16, 16, 4,
            GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0));
    }
    gsKit_set_test(gs, GS_ATEST_ON);
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);

    for (int e = 0; e < 6; e++) if (enemies[e].active) {
        u64 enemyColor = enemies[e].flash ? GS_SETREG_RGBAQ(255, 255, 255, 0x80, 0) : GS_SETREG_RGBAQ(170, 65, 65, 0x80, 0);
        gsKit_prim_sprite(gs, enemies[e].x - 10, enemies[e].y - 10, enemies[e].x + 10, enemies[e].y + 10, 4, enemyColor);
    }

    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ATEST_OFF);
    u64 playerColor = (damageCooldown > 0 && (damageCooldown & 4)) ? GS_SETREG_RGBAQ(255, 100, 100, 0x80, 0) : GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0);
    gsKit_prim_sprite_texture(gs, &isaacTextures[facing][walkFrame],
        isaac.x - 16, isaac.y - 24, 0, 0, isaac.x + 16, isaac.y + 24, 32, 48, 5, playerColor);
    gsKit_set_test(gs, GS_ATEST_ON);
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);

    for (int i = 0; i < 16; i++) if (tears[i].active)
        gsKit_prim_sprite(gs, tears[i].x - 4, tears[i].y - 4, tears[i].x + 4, tears[i].y + 4, 6, tearColor);

    // Simple heart HUD: one red half-heart block per two health units.
    for (int h = 0; h < 3; h++) {
        u64 c = (isaac.health >= (h + 1) * 2) ? GS_SETREG_RGBAQ(205, 55, 65, 0x80, 0) :
                (isaac.health > h * 2 ? GS_SETREG_RGBAQ(205, 55, 65, 0x80, 0) : GS_SETREG_RGBAQ(55, 45, 48, 0x80, 0));
        gsKit_prim_sprite(gs, roomLeft + 8 + h * 15, roomTop + 17, roomLeft + 19 + h * 15, roomTop + 27, 7, c);
    }
    // Original pickup atlas icons from assets/ui/hudpickups.png.
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ATEST_OFF);
    gsKit_prim_sprite_texture(gs, &pickupTextures[1], roomLeft + 10, roomBottom - 22, 0, 0, roomLeft + 26, roomBottom - 6, 16, 16, 8, GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
    gsKit_prim_sprite_texture(gs, &pickupTextures[2], roomLeft + 48, roomBottom - 22, 0, 0, roomLeft + 64, roomBottom - 6, 16, 16, 8, GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
    gsKit_prim_sprite_texture(gs, &pickupTextures[3], roomLeft + 86, roomBottom - 22, 0, 0, roomLeft + 102, roomBottom - 6, 16, 16, 8, GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
    gsKit_set_test(gs, GS_ATEST_ON);
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);
    for (int c = 0; c < (coins > 9 ? 9 : coins); c++)
        gsKit_prim_sprite(gs, roomLeft + 28 + c * 3, roomBottom - 18, roomLeft + 30 + c * 3, roomBottom - 14, 8, GS_SETREG_RGBAQ(235, 190, 55, 0x80, 0));

    // Room number and clear indicator are represented with compact status bars.
    u64 status = roomCleared ? GS_SETREG_RGBAQ(80, 190, 95, 0x80, 0) : GS_SETREG_RGBAQ(180, 135, 65, 0x80, 0);
    gsKit_prim_sprite(gs, roomRight - 30, roomTop + 17, roomRight - 8, roomTop + 22, 7, status);
}
