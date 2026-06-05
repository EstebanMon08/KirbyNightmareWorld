#pragma once
#include <stdbool.h>

/* ══════════════════════════════════════════════════════
   player.h — GameState, enums, constantes y funciones
               del jugador
   ══════════════════════════════════════════════════════ */

/* ── Modos del juego ── */
enum GameMode {
    MODE_MENU     = 0,
    MODE_PLAYING  = 1,
    MODE_GAMEOVER = 2
};

/* ── Poderes de Kirby ── */
enum PowerType {
    POWER_NONE  = 0,
    POWER_FIRE  = 1,
    POWER_BEAM  = 2,
    POWER_SWORD = 3
};

/* ── Constantes de física y combate ── */
#define INVINCIBLE_FRAMES   60
#define INHALE_RANGE        60
#define INHALE_PULL       5.0f
#define MOUTHFUL_MAX         1
#define ABYSS_LAUNCH      -4.0f

/* ── Struct GameState ── */
struct GameState {
    float worldX, y, velY, cameraX;
    bool  onGround, jumping, running;
    bool  keyLeft, keyRight, keyJump, keyInhale;
    bool  keySwallow;
    int   jumpCount;
    bool  jumpHeld;
    bool  eHeld;
    bool  swallowHeld;
    bool  inhaling;
    int   facingDir;
    int   hp, maxHp;
    int   invincibleTimer;
    int   mode;
    int   powerup;
    int   mouthEnemyType;
    int   kFireBurstLeft;
    int   kFireBurstDelay;
    int   kFireBurstDir;
    int   kBurstKind;
    float mouthX;
    float zoneX1, zoneX2, zoneY1, zoneY2;
};

extern GameState gState;

/* ── Funciones ── */
void  resetGame();
void  goToLevel(int n);

void* inputThread (void*);
void* kirbyThread (void*);
