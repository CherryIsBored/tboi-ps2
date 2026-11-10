#include "game.h"
#include "sprite_data.h"
#include "pickup_data.h"
#include "room_data.h"
#include "enemy_data.h"
#include "menu_data.h"
#include "heart_data.h"

static Player isaac;
static Tear tears[16];
static Enemy enemies[6];
static Pickup pickups[8];
static int coins = 0, keys = 0, bombs = 0;
static GSTEXTURE isaacTextures[4][10];
static GSTEXTURE isaacShootTextures[4];
static GSTEXTURE enemyTextures[4];
static GSTEXTURE pickupTextures[4];
static GSTEXTURE roomTexture;
static GSTEXTURE heartTexture;
static GSTEXTURE menuBackgroundTexture, menuLogoTexture, menuPromptTexture;

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
static int gameStarted = 0;
static int shootTimer = 0;
static int menuTick = 0;
static int enemyAnimTick = 0;
static unsigned int roomRng = 0x41C64E6Du;
static unsigned int room_random() {
    roomRng = roomRng * 1664525u + 1013904223u;
    return roomRng;
}

static float absf(float v) { return v < 0.0f ? -v : v; }

static void init_texture(GSGLOBAL* gs, GSTEXTURE* texture, int width, int height, const uint32_t* pixels) {
    texture->Width = width;
    texture->Height = height;
    texture->PSM = GS_PSM_CT32;
    texture->ClutPSM = GS_PSM_CT32;
    texture->Mem = (u32*)pixels;
    texture->Clut = NULL;
    texture->VramClut = 0;
    texture->Filter = GS_FILTER_NEAREST;
    texture->Delayed = GS_SETTING_OFF;
    texture->Vram = gsKit_vram_alloc(gs, gsKit_texture_size(width, height, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
    gsKit_texture_upload(gs, texture);
}

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
    // Mirrors the original room-reset lifecycle: clear old entities, then populate.
    for (int i = 0; i < 6; i++) enemies[i].active = 0;
    for (int i = 0; i < 16; i++) tears[i].active = 0;
    for (int i = 0; i < 8; i++) pickups[i].active = 0;
    roomRng ^= (unsigned int)(roomNumber * 0x45D9F3Bu);
    int count = 2 + (int)(room_random() % 3u);
    enemiesRemaining = count;
    roomCleared = 0;
    for (int i = 0; i < count; i++) {
        float x = roomLeft + 42.0f + (float)(room_random() % 207u);
        float y = roomTop + 38.0f + (float)(room_random() % 145u);
        if (absf(x - isaac.x) < 48.0f && absf(y - isaac.y) < 48.0f) {
            x = (x < isaac.x) ? roomLeft + 48.0f : roomRight - 48.0f;
            y = (y < isaac.y) ? roomTop + 42.0f : roomBottom - 42.0f;
        }
        enemies[i].x = x;
        enemies[i].y = y;
        enemies[i].hp = 2 + (roomNumber > 3 ? 1 : 0);
        enemies[i].speed = 0.48f + (float)(room_random() % 25u) * 0.01f;
        enemies[i].active = 1;
        enemies[i].flash = 0;
    }
}

void game_init(int width, int height, GSGLOBAL* gs) {
    // Restore the original centered playfield and its visible frame.
    roomLeft = width / 2.0f - 145.0f;
    roomRight = width / 2.0f + 145.0f;
    roomTop = height / 2.0f - 110.0f;
    roomBottom = height / 2.0f + 110.0f;

    isaac.x = width / 2.0f;
    isaac.y = height / 2.0f;
    isaac.speed = 2.5f;
    isaac.vx = isaac.vy = 0.0f;
    isaac.health = 6;
    tearCooldown = damageCooldown = 0;
    coins = keys = bombs = 0;
    for (int i = 0; i < 8; i++) pickups[i].active = 0;
    facing = walkFrame = walkTick = 0;
    roomNumber = 1;
    roomRng = 0x41C64E6Du;
    hitFlash = 0;
    gameStarted = 0;
    shootTimer = 0;
    menuTick = 0;
    enemyAnimTick = 0;

    for (int i = 0; i < 16; i++) tears[i].active = 0;

    for (int d = 0; d < 4; d++) {
        for (int f = 0; f < 10; f++)
            init_texture(gs, &isaacTextures[d][f], 32, 48, isaac_pixels[d][f]);
        init_texture(gs, &isaacShootTextures[d], 32, 48, isaac_shoot_pixels[d]);
    }
    init_texture(gs, &roomTexture, 256, 256, room_pixels);
    for (int e = 0; e < 4; e++)
        init_texture(gs, &enemyTextures[e], 32, 32, enemy_pixels[e]);
    for (int p = 0; p < 4; p++)
        init_texture(gs, &pickupTextures[p], 16, 16, pickup_pixels[p]);
    init_texture(gs, &heartTexture, 16, 16, heart_pixels);
    init_texture(gs, &menuBackgroundTexture, 256, 256, menu_bg_pixels);
    init_texture(gs, &menuLogoTexture, 256, 128, menu_logo_pixels);
    init_texture(gs, &menuPromptTexture, 128, 128, menu_prompt_pixels);
    spawn_room();
}

void game_update(int moveX, int moveY, int fireX, int fireY, int startPressed) {
    if (!gameStarted) {
        menuTick++;
        if (startPressed) {
            gameStarted = 1;
            spawn_room();
        }
        return;
    }

    if (shootTimer > 0) shootTimer--;
    enemyAnimTick++;

    float dx = (float)moveX, dy = (float)moveY;
    if (dx != 0 && dy != 0) { dx *= 0.7071f; dy *= 0.7071f; }

    if (fireX || fireY) {
        if (absf((float)fireX) > absf((float)fireY)) facing = fireX < 0 ? 2 : 3;
        else if (fireY) facing = fireY < 0 ? 1 : 0;
        shootTimer = 8;
        walkFrame = 0;
        walkTick = 0;
    } else {
        if (moveX < 0) facing = 2;
        else if (moveX > 0) facing = 3;
        else if (moveY < 0) facing = 1;
        else if (moveY > 0) facing = 0;
    }

    if (moveX || moveY) {
        if (shootTimer == 0 && ++walkTick >= 2) { walkTick = 0; walkFrame = (walkFrame + 1) % 10; }
    } else if (shootTimer == 0) { walkFrame = 0; walkTick = 0; }

    // Preserve momentum instead of snapping to the input direction each frame.
    isaac.vx = isaac.vx * 0.72f + dx * isaac.speed * 0.28f;
    isaac.vy = isaac.vy * 0.72f + dy * isaac.speed * 0.28f;
    if (!moveX) isaac.vx *= 0.82f;
    if (!moveY) isaac.vy *= 0.82f;
    isaac.x += isaac.vx;
    isaac.y += isaac.vy;
    if (isaac.x < roomLeft + 20) { isaac.x = roomLeft + 20; isaac.vx = 0; }
    if (isaac.x > roomRight - 20) { isaac.x = roomRight - 20; isaac.vx = 0; }
    if (isaac.y < roomTop + 20) { isaac.y = roomTop + 20; isaac.vy = 0; }
    if (isaac.y > roomBottom - 20) { isaac.y = roomBottom - 20; isaac.vy = 0; }

    if (tearCooldown > 0) tearCooldown--;
    if ((fireX || fireY) && tearCooldown == 0) {
        // Port of plff(): normalize aim, inherit part of Isaac's velocity,
        // and offset the tear perpendicular to its trajectory.
        float fx = (float)fireX;
        float fy = (float)fireY;
        float aimLen = absf(fx) + absf(fy);
        if (aimLen > 0.0f) {
            fx = fx / aimLen * 10.0f;
            fy = fy / aimLen * 10.0f;
        }
        float shotX = fx + isaac.vx * 0.6f;
        float shotY = fy + isaac.vy * 0.6f;
        float shotLen = absf(shotX) + absf(shotY);
        if (shotLen < 10.0f && shotLen > 0.01f) {
            shotX *= 10.0f / shotLen;
            shotY *= 10.0f / shotLen;
        }
        for (int i = 0; i < 16; i++) if (!tears[i].active) {
            tears[i].x = isaac.x - shotY * 0.4f;
            tears[i].y = isaac.y + shotX * 0.4f;
            tears[i].vx = shotX;
            tears[i].vy = shotY;
            tears[i].life = 75;
            tears[i].active = 1;
            tearCooldown = 10;
            break;
        }
    }

    for (int i = 0; i < 16; i++) {
        if (!tears[i].active) continue;
        tears[i].x += tears[i].vx;
        tears[i].y += tears[i].vy;
        if (tears[i].life > 0) tears[i].life--;
        if (tears[i].life <= 0 || tears[i].x < roomLeft + 12 || tears[i].x > roomRight - 12 ||
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
        float len2 = ex * ex + ey * ey;
        if (len2 > 1.0f) {
            float invLen = 1.0f / __builtin_sqrtf(len2);
            enemies[e].x += ex * invLen * enemies[e].speed;
            enemies[e].y += ey * invLen * enemies[e].speed;
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

    // Advance only through one of the four centered doorways, and only after clearing.
    if (roomCleared) {
        int nextRoom = 0;
        float centerX = (roomLeft + roomRight) * 0.5f;
        float centerY = (roomTop + roomBottom) * 0.5f;
        if (isaac.y < roomTop + 25.0f && absf(isaac.x - centerX) < 22.0f) {
            nextRoom = 1; isaac.y = roomBottom - 34.0f;
        } else if (isaac.y > roomBottom - 25.0f && absf(isaac.x - centerX) < 22.0f) {
            nextRoom = 1; isaac.y = roomTop + 34.0f;
        } else if (isaac.x < roomLeft + 25.0f && absf(isaac.y - centerY) < 22.0f) {
            nextRoom = 1; isaac.x = roomRight - 34.0f;
        } else if (isaac.x > roomRight - 25.0f && absf(isaac.y - centerY) < 22.0f) {
            nextRoom = 1; isaac.x = roomLeft + 34.0f;
        }
        if (nextRoom) {
            roomNumber++;
            isaac.vx = isaac.vy = 0.0f;
            spawn_room();
        }
    }
}

void game_draw(GSGLOBAL* gs) {
    u64 background = GS_SETREG_RGBAQ(20, 17, 24, 0x80, 0);
    u64 tearColor = GS_SETREG_RGBAQ(100, 190, 255, 0x80, 0);
    gsKit_clear(gs, background);

    if (!gameStarted) {
        gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
        gsKit_set_test(gs, GS_ATEST_OFF);
        gsKit_prim_sprite_texture(gs, &menuBackgroundTexture, 0, (gs->Height - 363) * 0.5f, 0, 0,
            gs->Width, (gs->Height + 363) * 0.5f, 256, 145, 1, GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
        gsKit_prim_sprite_texture(gs, &menuLogoTexture, gs->Width * 0.5f - 160, gs->Height * 0.22f, 0, 0,
            gs->Width * 0.5f + 160, gs->Height * 0.22f + 128, 256, 128, 2, GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
        if ((menuTick / 20) % 2 == 0) {
            gsKit_prim_sprite_texture(gs, &menuPromptTexture, gs->Width * 0.5f - 62, gs->Height * 0.66f, 0, 0,
                gs->Width * 0.5f + 62, gs->Height * 0.66f + 108, 125, 128, 3, GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
        }
        gsKit_set_test(gs, GS_ATEST_ON);
        gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);
        return;
    }

    // Explicitly repaint the whole gameplay frame before drawing the room.
    // This prevents the title-screen framebuffer from showing through if the
    // GS texture/alpha state survives a flip on real hardware.
    gsKit_set_test(gs, GS_ATEST_OFF);
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_prim_sprite(gs, 0, 0, gs->Width, gs->Height, 0,
        GS_SETREG_RGBAQ(20, 17, 24, 0x80, 0));

    gsKit_prim_sprite_texture(gs, &roomTexture, roomLeft, roomTop, 0, 0, roomRight, roomBottom, 256, 256, 1, GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));

    // Doors are physically represented only after every enemy is defeated.
    if (roomCleared) {
    // Four door openings are drawn strictly inside the wall thickness.
    // Each is a dark passage with a brown lintel and side jambs; none reaches
    // into the walkable floor, and all four are centered on their wall.
    const u64 doorDark = GS_SETREG_RGBAQ(8, 6, 7, 0x80, 0);
    const u64 doorFrame = GS_SETREG_RGBAQ(91, 59, 43, 0x80, 0);
    float cx = (roomLeft + roomRight) * 0.5f;
    float cy = (roomTop + roomBottom) * 0.5f;
    // North / south openings (wall thickness ~34 screen pixels).
    gsKit_prim_sprite(gs, cx - 13, roomTop + 4, cx + 13, roomTop + 30, 2, doorDark);
    gsKit_prim_sprite(gs, cx - 16, roomTop + 2, cx - 12, roomTop + 32, 2, doorFrame);
    gsKit_prim_sprite(gs, cx + 12, roomTop + 2, cx + 16, roomTop + 32, 2, doorFrame);
    gsKit_prim_sprite(gs, cx - 16, roomTop + 2, cx + 16, roomTop + 6, 2, doorFrame);
    gsKit_prim_sprite(gs, cx - 13, roomBottom - 30, cx + 13, roomBottom - 4, 2, doorDark);
    gsKit_prim_sprite(gs, cx - 16, roomBottom - 32, cx - 12, roomBottom - 2, 2, doorFrame);
    gsKit_prim_sprite(gs, cx + 12, roomBottom - 32, cx + 16, roomBottom - 2, 2, doorFrame);
    gsKit_prim_sprite(gs, cx - 16, roomBottom - 6, cx + 16, roomBottom - 2, 2, doorFrame);
    // West / east openings (wall thickness ~45 screen pixels).
    gsKit_prim_sprite(gs, roomLeft + 5, cy - 13, roomLeft + 35, cy + 13, 2, doorDark);
    gsKit_prim_sprite(gs, roomLeft + 3, cy - 16, roomLeft + 33, cy - 12, 2, doorFrame);
    gsKit_prim_sprite(gs, roomLeft + 3, cy + 12, roomLeft + 33, cy + 16, 2, doorFrame);
    gsKit_prim_sprite(gs, roomLeft + 3, cy - 16, roomLeft + 7, cy + 16, 2, doorFrame);
    gsKit_prim_sprite(gs, roomRight - 35, cy - 13, roomRight - 5, cy + 13, 2, doorDark);
    gsKit_prim_sprite(gs, roomRight - 33, cy - 16, roomRight - 3, cy - 12, 2, doorFrame);
    gsKit_prim_sprite(gs, roomRight - 33, cy + 12, roomRight - 3, cy + 16, 2, doorFrame);
    gsKit_prim_sprite(gs, roomRight - 7, cy - 16, roomRight - 3, cy + 16, 2, doorFrame);

    }

    gsKit_set_test(gs, GS_ATEST_ON);
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);

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

    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ATEST_OFF);
    for (int e = 0; e < 6; e++) if (enemies[e].active) {
        int frame = (enemyAnimTick / 5 + e) % 4;
        u64 tint = enemies[e].flash ? GS_SETREG_RGBAQ(255, 255, 255, 0x80, 0) : GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0);
        gsKit_prim_sprite_texture(gs, &enemyTextures[frame],
            enemies[e].x - 14, enemies[e].y - 14, 0, 0,
            enemies[e].x + 14, enemies[e].y + 14, 32, 32, 4, tint);
    }
    gsKit_set_test(gs, GS_ATEST_ON);
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);

    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ATEST_OFF);
    u64 playerColor = (damageCooldown > 0 && (damageCooldown & 4)) ? GS_SETREG_RGBAQ(255, 100, 100, 0x80, 0) : GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0);
    GSTEXTURE* playerTexture = shootTimer > 0 ? &isaacShootTextures[facing] : &isaacTextures[facing][walkFrame];
    gsKit_prim_sprite_texture(gs, playerTexture,
        isaac.x - 16, isaac.y - 24, 0, 0, isaac.x + 16, isaac.y + 24, 32, 48, 5, playerColor);
    gsKit_set_test(gs, GS_ATEST_ON);
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);

    // Simple, unmistakable blue tear shots until BulletAtlas.png is added.
    // Keep alpha testing disabled for these untextured primitives.
    gsKit_set_test(gs, GS_ATEST_OFF);
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    for (int i = 0; i < 16; i++) if (tears[i].active) {
        gsKit_prim_sprite(gs, tears[i].x - 4, tears[i].y - 4,
            tears[i].x + 4, tears[i].y + 4, 6,
            GS_SETREG_RGBAQ(55, 145, 255, 0x80, 0));
    }
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);
    gsKit_set_test(gs, GS_ATEST_ON);

    // Original heart icon from ui_hearts.png.
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ATEST_OFF);
    for (int h = 0; h < 3; h++) {
        u64 c = isaac.health > h * 2 ? GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0) : GS_SETREG_RGBAQ(75, 75, 75, 0x80, 0);
        gsKit_prim_sprite_texture(gs, &heartTexture, roomLeft + 8 + h * 19, roomTop + 13,
            0, 0, roomLeft + 24 + h * 19, roomTop + 29, 16, 16, 7, c);
    }
    gsKit_set_test(gs, GS_ATEST_ON);
    gsKit_set_primalpha(gs, GS_BLEND_BACK2FRONT, 0);
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
