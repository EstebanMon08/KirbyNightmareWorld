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
   player.cpp — GameState, física de Kirby,
                kirbyThread e inputThread
   Kirby Dreamland (ncurses)
   ══════════════════════════════════════════════════════ */

GameState gState;

static int noLeftCount    = 99;
static int noRightCount   = 99;
static int noJumpCount    = 99;
static int noInhaleCount  = 99;
static int noSwallowCount = 99;
const  int RELEASE_THRESHOLD = 8;


/* ════════════════════════════════
   resetGame
   ════════════════════════════════ */
void resetGame() {
    currentLevel = 1;
    initLevel();
    gState.worldX          = 10;
    gState.y               = GROUND_Y - KIRBY_H;
    gState.velY            = 0;
    gState.cameraX         = 0;
    gState.onGround        = true;
    gState.jumping         = false;
    gState.keyLeft         = false; gState.keyRight  = false;
    gState.keyJump         = false; gState.keyInhale = false;
    gState.inhaling        = false; gState.eHeld     = false;
    gState.keySwallow      = false; gState.swallowHeld = false;
    gState.jumpCount       = 0;     gState.jumpHeld  = false;
    gState.facingDir       = 1;
    gState.hp              = gState.maxHp;
    gState.invincibleTimer = 0;
    while (sem_trywait(&semMouthful) == 0) {}
    gStar.active           = false;
    gState.powerup         = POWER_NONE;
    gState.mouthEnemyType  = -1;
    gState.kFireBurstLeft  = 0; gState.kFireBurstDelay = 0;
    gState.kFireBurstDir   = 1; gState.kBurstKind = POWER_NONE;
    for (int i = 0; i < MAX_KIRBY_SHOTS; i++) kirbyShots[i].active = false;
    initEnemies();
}


/* ════════════════════════════════
   goToLevel
   Llamar solo con los 5 candados tomados en orden.
   ════════════════════════════════ */
void goToLevel(int n) {
    currentLevel = n;
    initLevel();
    initEnemies();
    gState.worldX     = 10;
    gState.y          = GROUND_Y - KIRBY_H;
    gState.velY       = 0;   gState.cameraX  = 0;
    gState.onGround   = true; gState.jumping  = false;
    gState.jumpCount  = 0;   gState.jumpHeld = false;
    gState.facingDir  = 1;
    while (sem_trywait(&semMouthful) == 0) {}
    gState.powerup        = POWER_NONE; gState.mouthEnemyType = -1;
    gState.kFireBurstLeft = 0;          gState.kFireBurstDelay = 0;
    gState.kFireBurstDir  = 1;          gState.kBurstKind = POWER_NONE;
    for (int i = 0; i < MAX_KIRBY_SHOTS; i++) kirbyShots[i].active = false;
    gStar.active = false;
}


/* ════════════════════════════════
   inputThread
   ════════════════════════════════ */
void* inputThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        bool run = gState.running;
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        int ch;
        bool sawLeft=false, sawRight=false, sawJump=false;
        bool sawInhale=false, sawSwallow=false;
        bool sawConfirm=false, sawQuit=false;

        pthread_mutex_lock(&ncMutex);
        while ((ch = getch()) != ERR) {
            switch (ch) {
                case 'a': case 'A': sawLeft    = true; break;
                case 'd': case 'D': sawRight   = true; break;
                case 'w': case 'W': case ' ':  sawJump    = true; break;
                case 'e': case 'E': sawInhale  = true; break;
                case 's': case 'S': sawSwallow = true; break;
                case '\n': case '\r': case KEY_ENTER: sawConfirm = true; break;
                case 'q': case 'Q': case 27: sawQuit = true; break;
            }
        }
        pthread_mutex_unlock(&ncMutex);

        if (sawQuit) {
            pthread_mutex_lock(&gMutex);
            gState.running = false;
            pthread_cond_broadcast(&gModeCond);
            pthread_mutex_unlock(&gMutex);
            sem_post(&semSpit);
            break;
        }

        if (sawLeft)    noLeftCount    = 0; else noLeftCount++;
        if (sawRight)   noRightCount   = 0; else noRightCount++;
        if (sawJump)    noJumpCount    = 0; else noJumpCount++;
        if (sawInhale)  noInhaleCount  = 0; else noInhaleCount++;
        if (sawSwallow) noSwallowCount = 0; else noSwallowCount++;

        pthread_mutex_lock(&gMutex);
        int m = gState.mode;
        pthread_mutex_unlock(&gMutex);

        if (m == MODE_PLAYING) {
            pthread_mutex_lock(&playerMutex);
            gState.keyLeft    = (noLeftCount    < RELEASE_THRESHOLD);
            gState.keyRight   = (noRightCount   < RELEASE_THRESHOLD);
            gState.keyJump    = (noJumpCount    < RELEASE_THRESHOLD);
            gState.keyInhale  = (noInhaleCount  < RELEASE_THRESHOLD);
            gState.keySwallow = (noSwallowCount < RELEASE_THRESHOLD);
            pthread_mutex_unlock(&playerMutex);
        } else if (sawConfirm) {
            pthread_mutex_lock(&gMutex);
            pthread_mutex_lock(&playerMutex);
            pthread_mutex_lock(&enemyMutex);
            pthread_mutex_lock(&projMutex);
            pthread_mutex_lock(&ncMutex);
            resetGame();
            gState.mode = MODE_PLAYING;
            pthread_cond_broadcast(&gModeCond);
            pthread_mutex_unlock(&ncMutex);
            pthread_mutex_unlock(&projMutex);
            pthread_mutex_unlock(&enemyMutex);
            pthread_mutex_unlock(&playerMutex);
            pthread_mutex_unlock(&gMutex);
        }

        usleep(8000);
    }
    return nullptr;
}


/* ════════════════════════════════
   kirbyThread
   ════════════════════════════════ */
void* kirbyThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        while (gState.running && gState.mode != MODE_PLAYING)
            pthread_cond_wait(&gModeCond, &gMutex);
        bool run = gState.running;
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        bool reachedGoal = false;
        pthread_mutex_lock(&playerMutex);

        int curMouth;
        sem_getvalue(&semMouthful, &curMouth);
        bool mouthFull = (curMouth > 0);
        bool hasPower  = (gState.powerup != POWER_NONE);

        /* ── Tecla E (flanco) ── */
        if (gState.keyInhale && !gState.eHeld) {
            if (hasPower) {
                gState.kFireBurstLeft  = (gState.powerup == POWER_FIRE)  ? FLAME_BURST_COUNT :
                                         (gState.powerup == POWER_BEAM)  ? BEAM_BURST_COUNT  :
                                                                            SWORD_BURST_COUNT;
                gState.kFireBurstDelay = 0;
                gState.kFireBurstDir   = gState.facingDir;
                gState.kBurstKind      = gState.powerup;
            } else if (mouthFull) {
                if (sem_trywait(&semMouthful) == 0)
                    sem_post(&semSpit);
            }
        }
        gState.eHeld = gState.keyInhale;

        gState.inhaling = gState.keyInhale && gState.onGround && !mouthFull && !hasPower;
        bool firing     = hasPower && (gState.keyInhale || gState.kFireBurstLeft > 0);

        /* ── Tecla S (flanco) ── */
        if (gState.keySwallow && !gState.swallowHeld) {
            if (mouthFull) {
                if (sem_trywait(&semMouthful) == 0) {
                    if      (gState.mouthEnemyType == ENEMY_SHOOTER) gState.powerup = POWER_FIRE;
                    else if (gState.mouthEnemyType == ENEMY_BEAM)    gState.powerup = POWER_BEAM;
                    else if (gState.mouthEnemyType == ENEMY_SWORD)   gState.powerup = POWER_SWORD;
                    else                                              gState.powerup = POWER_NONE;
                    gState.mouthEnemyType = -1;
                }
            } else if (hasPower) {
                gState.powerup = POWER_NONE;
            }
            gState.swallowHeld = true;
        }
        if (!gState.keySwallow) gState.swallowHeld = false;

        /* ── Emitir ráfaga ── */
        if (gState.kFireBurstLeft > 0) {
            if (gState.kFireBurstDelay <= 0) {
                pthread_mutex_lock(&projMutex);
                if (gState.kBurstKind == POWER_FIRE) {
                    float fx = (gState.kFireBurstDir == 1) ? (gState.worldX + KIRBY_W - 5)
                                                           : (gState.worldX + 5);
                    float fy = gState.y + KIRBY_H / 2;
                    spawnKirbyShot(fx, fy, FLAME_SPEED * gState.kFireBurstDir, 0.0f, FLAME_LIFE);
                    gState.kFireBurstDelay = FLAME_BURST_GAP;
                } else if (gState.kBurstKind == POWER_BEAM) {
                    int   idx   = BEAM_BURST_COUNT - gState.kFireBurstLeft;
                    float t     = (BEAM_BURST_COUNT > 1) ? (float)idx / (BEAM_BURST_COUNT - 1) : 0.0f;
                    float theta = (float)(M_PI / 2.0) - (float)M_PI * t;
                    float origX = (gState.kFireBurstDir == 1) ? (gState.worldX + KIRBY_W - 5)
                                                              : (gState.worldX + 5);
                    float origY = gState.y + KIRBY_H / 2.0f;
                    spawnKirbyShot(origX + BEAM_RADIUS * cosf(theta) * gState.kFireBurstDir,
                                   origY - BEAM_RADIUS * sinf(theta), 0.0f, 0.0f, BEAM_LIFE);
                    gState.kFireBurstDelay = BEAM_BURST_GAP;
                } else {
                    if (gState.kFireBurstLeft == SWORD_BURST_COUNT) {
                        float sx = (gState.kFireBurstDir == 1)
                                       ? (gState.worldX + KIRBY_W + SWORD_SLASH_OFFSET)
                                       : (gState.worldX - SWORD_SLASH_OFFSET);
                        spawnKirbyShot(sx, gState.y + KIRBY_H / 2.0f, 0.0f, 0.0f, SWORD_SLASH_LIFE);
                    }
                    gState.kFireBurstDelay = SWORD_BURST_GAP;
                }
                pthread_mutex_unlock(&projMutex);
                gState.kFireBurstLeft--;
            } else {
                gState.kFireBurstDelay--;
            }
        }

        /* ── Movimiento ── */
        if (!gState.inhaling && !firing) {
            if (gState.keyLeft)  { gState.worldX -= 3.0f; gState.facingDir = -1; }
            if (gState.keyRight) { gState.worldX += 3.0f; gState.facingDir =  1; }
        }
        if (gState.worldX < 0)                   gState.worldX = 0;
        if (gState.worldX > WORLD_W - KIRBY_W)   gState.worldX = WORLD_W - KIRBY_W;

        /* ── Salto multi-nivel ── */
        if (gState.keyJump && !gState.jumpHeld && !gState.inhaling && !firing) {
            if (gState.onGround) {
                gState.velY = -3.0f; gState.onGround = false;
                gState.jumping = true; gState.jumpCount = 1;
            } else if (gState.jumpCount < 5) {
                gState.jumpCount++;
                gState.velY = -2.0f;
            }
            gState.jumpHeld = true;
        }
        if (!gState.keyJump) gState.jumpHeld = false;

        /* ── Gravedad ── */
        if (!gState.onGround) { gState.velY += 0.18f; gState.y += gState.velY; }

        /* ── Abismo ── */
        if (gState.y > SCREEN_H + 5) {
            gState.y        = (float)(SCREEN_H - KIRBY_H);
            gState.velY     = ABYSS_LAUNCH;
            gState.onGround = false; gState.jumping = true; gState.jumpCount = 0;
            if (gState.invincibleTimer == 0) {
                gState.hp--;
                gState.invincibleTimer = INVINCIBLE_FRAMES;
                if (gState.hp <= 0) gState.hp = 0;
            }
        }

        /* ── Colisión con terreno ── */
        gState.onGround = false;
        bool cayendo = (gState.velY >= 0);
        for (int i = 0; i < numGround && !gState.onGround; i++) {
            float bx = ground[i].x, by = ground[i].y;
            bool dentroX = (gState.worldX + KIRBY_W > bx) && (gState.worldX < bx + BLOCK_W);
            bool sobreY  = (gState.y + KIRBY_H >= by) && (gState.y + KIRBY_H <= by + 3);
            if (dentroX && cayendo && sobreY) {
                gState.y = by - KIRBY_H; gState.velY = 0;
                gState.onGround = true; gState.jumping = false; gState.jumpCount = 0;
            }
        }
        for (int i = 0; i < numPlats && !gState.onGround; i++) {
            float px = platforms[i].x, py = platforms[i].y;
            bool dentroX = (gState.worldX + KIRBY_W > px) && (gState.worldX < px + PLAT_W);
            bool sobreY  = (gState.y + KIRBY_H >= py) && (gState.y + KIRBY_H <= py + 3);
            if (dentroX && cayendo && sobreY) {
                gState.y = py - KIRBY_H; gState.velY = 0;
                gState.onGround = true; gState.jumping = false; gState.jumpCount = 0;
            }
        }

        /* ── Zona de succión ── */
        float bodyLeft  = gState.worldX + 5;
        float bodyRight = gState.worldX + KIRBY_W - 5;
        gState.mouthX = (gState.facingDir == 1) ? bodyRight : bodyLeft;
        gState.zoneY1 = gState.y + 3;
        gState.zoneY2 = gState.y + KIRBY_H - 3;
        if (gState.facingDir == 1) {
            gState.zoneX1 = bodyRight - 5;
            gState.zoneX2 = bodyRight + INHALE_RANGE;
        } else {
            gState.zoneX1 = bodyLeft - INHALE_RANGE;
            gState.zoneX2 = bodyLeft + 5;
        }

        /* ── Invencibilidad ── */
        if (gState.invincibleTimer > 0) gState.invincibleTimer--;

        /* ── Cámara ── */
        float targetCam = gState.worldX - SCREEN_W / 2.0f;
        if (targetCam < 0)              targetCam = 0;
        if (targetCam > WORLD_W - SCREEN_W) targetCam = WORLD_W - SCREEN_W;
        gState.cameraX += (targetCam - gState.cameraX) * 0.2f;

        /* ── Meta ── */
        if (goal.active) {
            bool ovX = (gState.worldX + KIRBY_W > goal.x) && (gState.worldX < goal.x + GOAL_W);
            bool ovY = (gState.y + KIRBY_H > goal.y) && (gState.y < goal.y + GOAL_H);
            if (ovX && ovY) reachedGoal = true;
        }

        pthread_mutex_unlock(&playerMutex);

        /* ── Transición de nivel ── */
        if (reachedGoal) {
            pthread_mutex_lock(&gMutex);
            pthread_mutex_lock(&playerMutex);
            pthread_mutex_lock(&enemyMutex);
            pthread_mutex_lock(&projMutex);
            pthread_mutex_lock(&ncMutex);
            if (currentLevel >= 2) gState.mode = MODE_MENU;
            else                   goToLevel(currentLevel + 1);
            pthread_mutex_unlock(&ncMutex);
            pthread_mutex_unlock(&projMutex);
            pthread_mutex_unlock(&enemyMutex);
            pthread_mutex_unlock(&playerMutex);
            pthread_mutex_unlock(&gMutex);
        }

        usleep(33333);
    }
    return nullptr;
}
