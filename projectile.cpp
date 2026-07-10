#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include "sync.h"
#include "sprites.h"
#include "level.h"
#include "enemy.h"
#include "projectile.h"
#include "player.h"

/* ══════════════════════════════════════════════════════
   projectile.cpp — Pools de proyectiles y sus hilos
   Kirby Dreamland (ncurses)
   ══════════════════════════════════════════════════════ */

Star         gStar            = { 0, 0, 1, false, 4.0f };
const int    MAX_KIRBY_SHOTS  = 32;
KirbyShot    kirbyShots[32];


/* ════════════════════════════════
   spawnKirbyShot  (llamar con projMutex tomado)
   ════════════════════════════════ */
void spawnKirbyShot(float x, float y, float vx, float vy, int life, int shotType, int dir) {
    for (int i = 0; i < MAX_KIRBY_SHOTS; i++) {
        if (!kirbyShots[i].active) {
            kirbyShots[i] = { x, y, vx, vy, true, life, shotType, dir };
            return;
        }
    }
}


/* ════════════════════════════════
   starThread
   ════════════════════════════════ */
void* starThread(void*) {
    while (true) {
        sem_wait(&semSpit);

        pthread_mutex_lock(&gMutex);
        bool run = gState.running;
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        pthread_mutex_lock(&playerMutex);
        pthread_mutex_lock(&projMutex);
        gStar.dir    = gState.facingDir;
        gStar.x      = (gStar.dir == 1) ? gState.worldX + KIRBY_W - 5
                                        : gState.worldX + 5;
        gStar.y      = gState.y + KIRBY_H / 2;
        gStar.active = true;
        pthread_mutex_unlock(&projMutex);
        pthread_mutex_unlock(&playerMutex);

        bool flying = true;
        while (flying) {
            pthread_mutex_lock(&gMutex);
            bool stillRun = gState.running;
            pthread_mutex_unlock(&gMutex);
            if (!stillRun) return nullptr;

            pthread_mutex_lock(&playerMutex);
            pthread_mutex_lock(&enemyMutex);
            pthread_mutex_lock(&projMutex);

            gStar.x += gStar.speed * gStar.dir;

            for (int i = 0; i < NUM_ENEMIES; i++) {
                if (!enemies[i].alive) continue;
                Enemy& e = enemies[i];
                if (gStar.x >= e.x - 2 && gStar.x <= e.x + e.w + 2 &&
                    gStar.y >= e.y - 1 && gStar.y <= e.y + e.h) {
                    enemies[i].alive = false;
                    gStar.active     = false;
                    flying           = false;
                }
            }

            float screenX = gStar.x - gState.cameraX;
            if (screenX < -2 || screenX > SCREEN_W + 2) {
                gStar.active = false;
                flying       = false;
            }

            pthread_mutex_unlock(&projMutex);
            pthread_mutex_unlock(&enemyMutex);
            pthread_mutex_unlock(&playerMutex);

            if (flying) usleep(33333);
        }
    }
    return nullptr;
}


/* ════════════════════════════════
   kirbyShotThread
   ════════════════════════════════ */
void* kirbyShotThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        while (gState.running && gState.mode != MODE_PLAYING)
            pthread_cond_wait(&gModeCond, &gMutex);
        bool run = gState.running;
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        pthread_mutex_lock(&playerMutex);
        float camX = gState.cameraX;
        pthread_mutex_unlock(&playerMutex);

        pthread_mutex_lock(&enemyMutex);
        pthread_mutex_lock(&projMutex);
        for (int i = 0; i < MAX_KIRBY_SHOTS; i++) {
            if (!kirbyShots[i].active) continue;
            KirbyShot& ks = kirbyShots[i];
            ks.x += ks.vx;
            ks.y += ks.vy;
            ks.life--;
            if (ks.life <= 0) { ks.active = false; continue; }
            float sx = ks.x - camX;
            if (sx < -2 || sx > SCREEN_W + 2) { ks.active = false; continue; }
            for (int j = 0; j < NUM_ENEMIES; j++) {
                if (!enemies[j].alive) continue;
                Enemy& en = enemies[j];
                if (ks.x >= en.x - 2 && ks.x <= en.x + en.w + 2 &&
                    ks.y >= en.y - 1 && ks.y <= en.y + en.h) {
                    en.alive  = false;
                    ks.active = false;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&projMutex);
        pthread_mutex_unlock(&enemyMutex);

        usleep(33333);
    }
    return nullptr;
}


/* ════════════════════════════════
   enemyShotThread
   ════════════════════════════════ */
void* enemyShotThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        while (gState.running && gState.mode != MODE_PLAYING)
            pthread_cond_wait(&gModeCond, &gMutex);
        bool run = gState.running;
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        pthread_mutex_lock(&playerMutex);
        pthread_mutex_lock(&projMutex);

        float camX = gState.cameraX;

        for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
            if (!enemyShots[i].active) continue;
            EnemyShot& s = enemyShots[i];
            s.x += s.vx;
            s.y += s.vy;
            s.life--;
            if (s.life <= 0) { s.active = false; continue; }
            float sx = s.x - camX;
            if (sx < -2 || sx > SCREEN_W + 2) { s.active = false; continue; }

            float kx1 = gState.worldX + 5,         ky1 = gState.y + 3;
            float kx2 = gState.worldX + KIRBY_W - 5, ky2 = gState.y + KIRBY_H;
            if (s.x >= kx1 && s.x <= kx2 && s.y >= ky1 && s.y <= ky2) {
                s.active = false;
                if (gState.invincibleTimer == 0) {
                    gState.hp--;
                    gState.invincibleTimer = INVINCIBLE_FRAMES;
                    gState.worldX += (s.dir == 1) ? 8.0f : -8.0f;
                    gState.velY    = -2.0f;
                    gState.onGround = false;
                    gState.jumping  = true;
                    gState.jumpCount = 0;
                    if (gState.hp <= 0) gState.hp = 0;
                }
            }
        }

        pthread_mutex_unlock(&projMutex);
        pthread_mutex_unlock(&playerMutex);

        usleep(33333);
    }
    return nullptr;
}
