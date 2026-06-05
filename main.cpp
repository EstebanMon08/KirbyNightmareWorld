#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* ── Headers del proyecto (deben ir antes de todo lo demás) ── */
#include "sync.h"
#include "sprites.h"
#include "level.h"
#include "enemy.h"
#include "projectile.h"
#include "player.h"
#include "render.h"

/* ══════════════════════════════════════════════════════
   main.cpp — Definición de variables globales y main()
   Kirby Dreamland (ncurses)

   SOLO este archivo define las variables globales.
   Todos los demás las declaran como extern via sync.h / sus headers.
   ══════════════════════════════════════════════════════ */

/* ── Tamaño de la terminal ── */
int SCREEN_W = 80;
int SCREEN_H = 24;

/* ── Nivel actual ── */
int currentLevel = 1;

/* ── Mutexes (orden de adquisición: gMutex->playerMutex->enemyMutex->projMutex->ncMutex) ── */
pthread_mutex_t gMutex      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t playerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t enemyMutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t projMutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ncMutex     = PTHREAD_MUTEX_INITIALIZER;

/* ── Variable de condición (emparejada con gMutex) ── */
pthread_cond_t gModeCond = PTHREAD_COND_INITIALIZER;

/* ── Semáforos ── */
sem_t semMouthful;
sem_t semSpit;


/* ════════════════════════════════
   main
   ════════════════════════════════ */
int main() {
    setlocale(LC_ALL, "");
    initscr(); cbreak(); noecho();
    curs_set(0); nodelay(stdscr, TRUE); keypad(stdscr, TRUE);

    getmaxyx(stdscr, SCREEN_H, SCREEN_W);

    initLevel();
    initEnemies();

    sem_init(&semMouthful, 0, 0);
    sem_init(&semSpit,     0, 0);

    /* Estado inicial de Kirby */
    gState.worldX          = 10;
    gState.y               = GROUND_Y - KIRBY_H;
    gState.velY            = 0;
    gState.cameraX         = 0;
    gState.onGround        = true;
    gState.jumping         = false;
    gState.running         = true;
    gState.keyLeft         = false;
    gState.keyRight        = false;
    gState.keyJump         = false;
    gState.keyInhale       = false;
    gState.inhaling        = false;
    gState.eHeld           = false;
    gState.keySwallow      = false;
    gState.swallowHeld     = false;
    gState.jumpCount       = 0;
    gState.jumpHeld        = false;
    gState.facingDir       = 1;
    gState.hp              = 6;
    gState.maxHp           = 6;
    gState.invincibleTimer = 0;
    gState.mode            = MODE_MENU;
    gState.powerup         = POWER_NONE;
    gState.mouthEnemyType  = -1;
    gState.kFireBurstLeft  = 0;
    gState.kFireBurstDelay = 0;
    gState.kFireBurstDir   = 1;
    gState.kBurstKind      = POWER_NONE;
    for (int i = 0; i < MAX_KIRBY_SHOTS; i++) kirbyShots[i].active = false;

    /* Lanzar hilos */
    pthread_t tIn, tKi, tEn, tKs, tRe, tSt, tEs, tDi;
    pthread_create(&tIn, nullptr, inputThread,     nullptr);
    pthread_create(&tKi, nullptr, kirbyThread,     nullptr);
    pthread_create(&tEn, nullptr, enemyThread,     nullptr);
    pthread_create(&tKs, nullptr, kirbyShotThread, nullptr);
    pthread_create(&tDi, nullptr, directorThread,  nullptr);
    pthread_create(&tRe, nullptr, renderThread,    nullptr);
    pthread_create(&tSt, nullptr, starThread,      nullptr);
    pthread_create(&tEs, nullptr, enemyShotThread, nullptr);

    pthread_join(tIn, nullptr);
    pthread_join(tKi, nullptr);
    pthread_join(tEn, nullptr);
    pthread_join(tKs, nullptr);
    pthread_join(tDi, nullptr);
    pthread_join(tRe, nullptr);
    pthread_join(tSt, nullptr);
    pthread_join(tEs, nullptr);

    pthread_mutex_destroy(&gMutex);
    pthread_mutex_destroy(&playerMutex);
    pthread_mutex_destroy(&enemyMutex);
    pthread_mutex_destroy(&projMutex);
    pthread_mutex_destroy(&ncMutex);
    pthread_cond_destroy(&gModeCond);
    sem_destroy(&semMouthful);
    sem_destroy(&semSpit);

    endwin();
    return 0;
}
