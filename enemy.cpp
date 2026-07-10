#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>

#include "sync.h"
#include "sprites.h"
#include "level.h"
#include "enemy.h"
#include "projectile.h"
#include "player.h"

/* ══════════════════════════════════════════════════════
   enemy.cpp — Struct Enemy, IA, hilo enemyThread
   Kirby Dreamland (ncurses)
   ══════════════════════════════════════════════════════ */

const int NUM_ENEMIES    = 5;
Enemy     enemies[NUM_ENEMIES];

const int MAX_ENEMY_SHOTS = 32;
EnemyShot enemyShots[MAX_ENEMY_SHOTS];


/* ════════════════════════════════
   initEnemies
   ════════════════════════════════ */
void initEnemies() {
    int groundY = SCREEN_H - 3;
    float gy = (float)(groundY - ENEMY_H);

    for (int i = 0; i < NUM_ENEMIES; i++) enemies[i].alive = false;
// Proyectil para ENEMY_SWORD
//  ▒▒▒▒▒▒▒▒▒▒▒▒               
// ▒▓▓▓         ▒▒▒▒           
//     ▓▓▓▓░░░     ▒▒▒▒▒       
//     ▒▒▓▓▓▓░░░       ▒▒▒     
//     ▓▓▒▒▓ ▓░░░       ▒▒▒▒   
//    ▒▒▒▓▓▒▓ ▓░░░   ░    ▒▒▒  
//   ▓▒▒▓▒▒▓ ▓ ▓░░░   ░ ▒   ▒▒ 
//    ▓▒ ▓▒  ▓ ▓░░░    ░ ▒  ▒▒▒
//    ▓        ▓░░░    ░ ▒   ▒▒
//    ▓▒ ▓▒  ▓ ▓░░░    ░ ▒  ▒▒▒
//   ▓▒▒▓▒▒▓ ▓ ▓░░░   ░ ▒   ▒▒ 
//    ▒▒▒▓▓▒▓ ▓░░░   ░    ▒▒▒  
//     ▓▓▒▒▓ ▓░░░       ▒▒▒▒   
//     ▒▒▓▓▓▓░░░       ▒▒▒     
//     ▓▓▓▓░░░     ▒▒▒▒▒       
// ▒▓▓▓         ▒▒▒▒           
//  ▒▒▒▒▒▒▒▒▒▒▒▒  

    if (currentLevel == 1) {
        enemies[0] = { 80,    gy,      ENEMY_W, ENEMY_H, true,  1, 1.0f, CUBE_DIR_FRAMES,    ENEMY_CUBE,    0, 0, 0 };
        enemies[1] = { 370,   gy-30,   ENEMY_W, ENEMY_H, true,  1, 0.3f, SHOOTER_DIR_FRAMES, ENEMY_SHOOTER, 0, 0, 0 };
        enemies[2] = { 500,   gy,      ENEMY_W, ENEMY_H, true, -1, 1.0f, CUBE_DIR_FRAMES,    ENEMY_CUBE,    0, 0, 0 };
        enemies[3] = { 700,   gy,      ENEMY_W, ENEMY_H, true,  1, 1.5f, SWORD_DIR_FRAMES,   ENEMY_SWORD,   0, 0, 0 };
        enemies[4] = { 695,   gy-42,   ENEMY_W, ENEMY_H, true, -1, 0.3f, SHOOTER_DIR_FRAMES, ENEMY_SHOOTER, 0, 0, 0 };
    }
    if (currentLevel == 2) {
        enemies[0] = { 275,  gy-40,   ENEMY_W, ENEMY_H, true,  1, 0.4f, CUBE_DIR_FRAMES,    ENEMY_BEAM,    0, 0, 0 };
        enemies[1] = { 720,  gy-48,   ENEMY_W, ENEMY_H, true, -1, 0.4f, BEAM_DIR_FRAMES,    ENEMY_BEAM,    0, 0, 0 };
        enemies[2] = { 1140, gy-24,   ENEMY_W, ENEMY_H, true, -1, 0.4f, BEAM_DIR_FRAMES,    ENEMY_BEAM,    0, 0, 0 };
    }

    for (int i = 0; i < MAX_ENEMY_SHOTS; i++) enemyShots[i].active = false;
}


/* ════════════════════════════════
   spawnEnemyShot  (llamar con projMutex tomado)
   ════════════════════════════════ */
void spawnEnemyShot(float x, float y, int dir, float vx, float vy, int life, int shotType) {
    for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
        if (!enemyShots[i].active) {
            enemyShots[i] = { x, y, vx, vy, dir, true, life, life, shotType };
            return;
        }
    }
}


/* ════════════════════════════════
   enemyThread
   ════════════════════════════════ */
void* enemyThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        while (gState.running && gState.mode != MODE_PLAYING)
            pthread_cond_wait(&gModeCond, &gMutex);
        bool run = gState.running;
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        pthread_mutex_lock(&playerMutex);
        pthread_mutex_lock(&enemyMutex);

        bool  inhaleActive = gState.inhaling;
        float mouthX = gState.mouthX;
        float zoneX1 = gState.zoneX1, zoneX2 = gState.zoneX2;
        float zoneY1 = gState.zoneY1, zoneY2 = gState.zoneY2;

        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];

            float ex1 = e.x,       ey1 = e.y;
            float ex2 = e.x + e.w, ey2 = e.y + e.h;

            /* ── Succión ── */
            bool enZona = inhaleActive &&
                          (ex1 < zoneX2) && (ex2 > zoneX1) &&
                          (ey1 < zoneY2) && (ey2 > zoneY1);
            if (enZona) {
                float enemyCenter = e.x + e.w / 2.0f;
                if (enemyCenter < mouthX) e.x += INHALE_PULL;
                else                      e.x -= INHALE_PULL;

                if (fabsf((e.x + e.w / 2.0f) - mouthX) < 6.0f) {
                    int cur;
                    sem_getvalue(&semMouthful, &cur);
                    if (cur < MOUTHFUL_MAX) {
                        e.alive = false;
                        gState.mouthEnemyType = e.type;
                        sem_post(&semMouthful);
                    }
                }
                continue;
            }

            /* ── Movimiento ── */
            bool isAttacker = (e.type == ENEMY_SHOOTER || e.type == ENEMY_BEAM ||
                               e.type == ENEMY_SWORD);
            bool bursting = (isAttacker && e.burstLeft > 0);
            if (!bursting) {
                e.x += e.speed * e.dir;
                e.dirTimer--;
                if (e.dirTimer <= 0) {
                    if      (e.type == ENEMY_SHOOTER) { e.burstLeft = FLAME_BURST_COUNT; e.burstDelay = 0; e.burstDir = e.dir; }
                    else if (e.type == ENEMY_BEAM)    { e.burstLeft = BEAM_BURST_COUNT;  e.burstDelay = 0; e.burstDir = e.dir; }
                    else if (e.type == ENEMY_SWORD)   { e.burstLeft = SWORD_BURST_COUNT; e.burstDelay = 0; e.burstDir = e.dir; }
                    e.dir *= -1;
                    if      (e.type == ENEMY_SHOOTER) e.dirTimer = SHOOTER_DIR_FRAMES;
                    else if (e.type == ENEMY_BEAM)    e.dirTimer = BEAM_DIR_FRAMES;
                    else if (e.type == ENEMY_SWORD)   e.dirTimer = SWORD_DIR_FRAMES;
                    else                              e.dirTimer = CUBE_DIR_FRAMES;
                }
            }

            /* ── Ráfaga de ataque ── */
            if (isAttacker && e.burstLeft > 0) {
                if (e.burstDelay <= 0) {
                    pthread_mutex_lock(&projMutex);
                    if (e.type == ENEMY_SHOOTER) {
                        float fx = (e.burstDir == 1) ? (e.x + e.w) : (e.x - 1);
                        float fy = e.y + e.h / 2.0f;
                        spawnEnemyShot(fx, fy, e.burstDir, FLAME_SPEED * e.burstDir, 0.0f, FLAME_LIFE);
                        e.burstDelay = FLAME_BURST_GAP;
                    } else if (e.type == ENEMY_BEAM) {
                        int   idx   = BEAM_BURST_COUNT - e.burstLeft;
                        float t     = (BEAM_BURST_COUNT > 1) ? (float)idx / (BEAM_BURST_COUNT - 1) : 0.0f;
                        float theta = (float)(M_PI / 2.0) - (float)M_PI * t;
                        float originX = (e.burstDir == 1) ? (e.x + e.w) : (e.x - 1);
                        float originY = e.y + e.h / 2.0f;
                        float px = originX + BEAM_RADIUS * cosf(theta) * e.burstDir;
                        float py = originY - BEAM_RADIUS * sinf(theta);
                        spawnEnemyShot(px, py, e.burstDir, 0.0f, 0.0f, BEAM_LIFE);
                        e.burstDelay = BEAM_BURST_GAP;
                    } else {
                        if (e.burstLeft == SWORD_BURST_COUNT) {
                            float sx = (e.burstDir == 1) ? (e.x + e.w + SWORD_SLASH_OFFSET)
                                                         : (e.x - SWORD_SLASH_OFFSET);
                            float sy = e.y + e.h / 2.0f;
                            spawnEnemyShot(sx, sy, e.burstDir, 0.0f, 0.0f, SWORD_SLASH_LIFE, ENEMY_SWORD);
                        }
                        e.burstDelay = SWORD_BURST_GAP;
                    }
                    pthread_mutex_unlock(&projMutex);
                    e.burstLeft--;
                } else {
                    e.burstDelay--;
                }
            }

            /* ── Colisión con Kirby ── */
            ex1 = e.x; ex2 = e.x + e.w;
            float kx1 = gState.worldX + 5,  ky1 = gState.y + 3;
            float kx2 = gState.worldX + KIRBY_W - 5, ky2 = gState.y + KIRBY_H - 3;
            bool col = (kx1 < ex2) && (kx2 > ex1) && (ky1 < ey2) && (ky2 > ey1);

            if (col && gState.invincibleTimer == 0) {
                gState.hp--;
                gState.invincibleTimer = INVINCIBLE_FRAMES;
                if (gState.worldX < e.x) gState.worldX -= 10.0f;
                else                     gState.worldX += 10.0f;
                gState.velY     = -2.0f;
                gState.onGround = false;
                gState.jumping  = true;
                gState.jumpCount = 0;
                if (gState.hp <= 0) gState.hp = 0;
            }
        }

        pthread_mutex_unlock(&enemyMutex);
        pthread_mutex_unlock(&playerMutex);

        usleep(33333);
    }
    return nullptr;
}
