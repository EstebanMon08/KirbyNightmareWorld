#include <locale.h>
#include <ncurses.h>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include "sync.h"
#include "sprites.h"
#include "level.h"
#include "enemy.h"
#include "projectile.h"
#include "player.h"
#include "render.h"

/* ══════════════════════════════════════════════════════
   render.cpp — renderThread y directorThread
   Kirby Dreamland (ncurses)
   ══════════════════════════════════════════════════════ */

#define FPS 30


/* ════════════════════════════════
   drawProjectile
   Dibuja la bola centrada en (cxScreen, cyScreen).
   Llamar con ncMutex tomado.
   ════════════════════════════════ */
void drawProjectile(int cxScreen, int cyScreen) {
    for (int r = 0; r < PROJ_H; r++) {
        int row = cyScreen - PROJ_H / 2 + r;
        if (row < 2 || row >= SCREEN_H) continue;
        const wchar_t* line = SPRITE_PROJECTILE[r];
        for (int c = 0; line[c] != L'\0'; c++) {
            if (line[c] == L' ') continue;
            int col = cxScreen - PROJ_W / 2 + c;
            if (col < 0 || col >= SCREEN_W) continue;
            wchar_t ch[2] = { line[c], L'\0' };
            mvaddwstr(row, col, ch);
        }
    }
}


/* ════════════════════════════════
   directorThread
   ════════════════════════════════ */
void* directorThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        while (gState.running && gState.mode != MODE_PLAYING)
            pthread_cond_wait(&gModeCond, &gMutex);
        bool run = gState.running;
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        pthread_mutex_lock(&gMutex);
        pthread_mutex_lock(&playerMutex);
        if (gState.hp <= 0 && gState.mode == MODE_PLAYING)
            gState.mode = MODE_GAMEOVER;
        pthread_mutex_unlock(&playerMutex);
        pthread_mutex_unlock(&gMutex);

        usleep(33333);
    }
    return nullptr;
}


/* ════════════════════════════════
   renderThread
   ════════════════════════════════ */
void* renderThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }
        bool isMenu     = (gState.mode == MODE_MENU);
        bool isGameOver = (gState.mode == MODE_GAMEOVER);
        pthread_mutex_unlock(&gMutex);

        pthread_mutex_lock(&playerMutex);
        float camX       = gState.cameraX;
        float kWorldX    = gState.worldX;
        float ky         = gState.y;
        float velY       = gState.velY;
        bool  jumping    = gState.jumping;
        bool  moving     = gState.keyLeft || gState.keyRight;
        int   facing     = gState.facingDir;
        int   hp         = gState.hp;
        int   maxHp      = gState.maxHp;
        int   invTimer   = gState.invincibleTimer;
        bool  isInhaling = gState.inhaling;
        int   powerup    = gState.powerup;
        pthread_mutex_unlock(&playerMutex);

        bool  starActive = gStar.active;
        float starX      = gStar.x;
        float starY      = gStar.y;

        int mouthful = 0;
        sem_getvalue(&semMouthful, &mouthful);
        bool stuffed = (mouthful > 0);

        pthread_mutex_lock(&ncMutex);
        erase();

        /* ── MENÚ ── */
        if (isMenu) {
            int top  = SCREEN_H / 2 - MENU_H / 2;
            int left = SCREEN_W / 2 - MENU_W / 2;
            for (int i = 0; i < MENU_H; i++) {
                int yy = top + i;
                if (yy >= 0 && yy < SCREEN_H && left >= 0)
                    mvaddwstr(yy, left, SPRITE_MENU[i]);
            }
            int optY = top + MENU_H + 1;
            int cx   = SCREEN_W / 2;
            mvprintw(optY,     cx - 7,  "__/\\__  INICIAR  __/\\__");
            mvprintw(optY + 1, cx - 7,  "\\    /  RANKING  \\    /");
            mvprintw(optY + 2, cx - 7,  "/_  _\\ CONTROLES /_  _\\");
            mvprintw(optY + 3, cx - 4,  "\\/    SALIR    \\/");
            mvprintw(optY + 5, cx - 13, "PRESIONA ENTER PARA JUGAR");
            mvprintw(optY + 6, cx -  9, "Q / ESC para salir");
            refresh();
            pthread_mutex_unlock(&ncMutex);
            usleep(1000000 / FPS);
            continue;
        }

        /* ── GAME OVER ── */
        if (isGameOver) {
            const char* go[] = {
                " █████    ██    ███    ███ ███████       █████  ███  ███ ███████ ██████   ",
                "██    █   █ ██    ███  ███   ██   █      ██   ██  ██   ██  ██   █  ██  ██ ",
                "██        █ ██    ██ █ █ █   ████        ██   ██   █  ██   ████    ██  ██ ",
                "██   ███ ██████   ██ ██  █   ██          ██   ██   ██ ██   ██      ████   ",
                "██    █  █   ██   ██     █   ██   █      ██   ██    ███    ██   █  ██  ██ ",
                "  █████ ███  ███ ███     ██ ███████       █████     ██    ███████ ███  ███"
            };
            int startY = SCREEN_H / 2 - 3;
            int startX = SCREEN_W / 2 - 37;
            for (int i = 0; i < 6; i++)
                mvprintw(startY + i, startX, "%s", go[i]);
            mvprintw(startY + 8, SCREEN_W / 2 - 16, "PRESIONA ENTER PARA REINICIAR");
            refresh();
            pthread_mutex_unlock(&ncMutex);
            usleep(1000000 / FPS);
            continue;
        }

        /* ── JUEGO ── */
        for (int i = 0; i < numGround; i++)
            drawBlock((int)(ground[i].x - camX), (int)ground[i].y);
        for (int i = 0; i < numPlats; i++)
            drawPlatform((int)(platforms[i].x - camX), (int)platforms[i].y);
        if (goal.active)
            drawGoal((int)(goal.x - camX), (int)goal.y);

        /* Enemigos */
        wchar_t eflipped[ENEMY_W + 10];
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];
            const wchar_t** esp;
            if      (e.type == ENEMY_SHOOTER) esp = SPRITE_SHOOTER;
            else if (e.type == ENEMY_BEAM)    esp = SPRITE_BEAM;
            else if (e.type == ENEMY_SWORD)   esp = SPRITE_SWORD;
            else                              esp = SPRITE_ENEMY;
            int ex   = (int)(e.x - camX);
            int edir = e.dir;
            if ((e.type == ENEMY_SHOOTER || e.type == ENEMY_BEAM ||
                 e.type == ENEMY_SWORD) && e.burstLeft > 0)
                edir = e.burstDir;
            for (int r = 0; r < e.h; r++) {
                int row = (int)e.y + r;
                if (row >= 2 && row < SCREEN_H && ex >= 0 && ex < SCREEN_W) {
                    if (edir == -1) { flipLine(esp[r], eflipped, ENEMY_W+10); mvaddwstr(row, ex, eflipped); }
                    else            { mvaddwstr(row, ex, esp[r]); }
                }
            }
        }

        /* Proyectiles enemigos */
        for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
            if (!enemyShots[i].active) continue;
            drawProjectile((int)(enemyShots[i].x - camX), (int)enemyShots[i].y);
        }

        /* Kirby */
        const wchar_t** sprite;
        if      (stuffed)              sprite = SPRITE_FULL;
        else if (isInhaling)           sprite = SPRITE_INHALE;
        else if (jumping && velY <  0) sprite = SPRITE_JUMP;
        else if (jumping && velY >= 0) sprite = SPRITE_FALL;
        else if (moving)               sprite = SPRITE_WALK;
        else                           sprite = SPRITE_IDLE;

        bool visible = (invTimer == 0) || ((invTimer / 4) % 2 == 0);
        int  kx      = (int)(kWorldX - camX);
        wchar_t flipped[KIRBY_W + 10];
        if (visible) {
            for (int r = 0; r < KIRBY_H; r++) {
                int row = (int)ky + r;
                if (sprite[r] == nullptr) continue;
                if (row >= 2 && row < SCREEN_H && kx >= 0 && kx < SCREEN_W) {
                    if (facing == -1) { flipLine(sprite[r], flipped, KIRBY_W+10); mvaddwstr(row, kx, flipped); }
                    else              { mvaddwstr(row, kx, sprite[r]); }
                }
            }
        }

        /* Efecto de succión */
        if (isInhaling) {
            static int windFrame = 0;
            windFrame++;
            const wchar_t* puff = L"\u00b7\u2218\u00b0";
            int mouthSX  = (facing == 1) ? (kx + KIRBY_W - 5) : (kx + 5);
            int rowBase  = (int)ky + 9;
            for (int d = 2; d < INHALE_RANGE; d += 3) {
                int wx = mouthSX + facing * d;
                int phase = (d + windFrame) % 3;
                for (int rr = 0; rr < 3; rr++) {
                    int row = rowBase + rr;
                    if (row >= 2 && row < SCREEN_H && wx >= 0 && wx < SCREEN_W) {
                        wchar_t g[2] = { puff[(phase+rr)%3], L'\0' };
                        mvaddwstr(row, wx, g);
                    }
                }
            }
        }

        /* Estrella y proyectiles de Kirby */
        if (starActive) drawProjectile((int)(starX - camX), (int)starY);
        for (int i = 0; i < MAX_KIRBY_SHOTS; i++) {
            if (!kirbyShots[i].active) continue;
            drawProjectile((int)(kirbyShots[i].x - camX), (int)kirbyShots[i].y);
        }

        /* HUD */
        mvprintw(0, 2, "KIRBY");
        for (int i = 0; i < maxHp; i++) {
            if (i < hp) { wchar_t f[] = L"■"; mvaddwstr(0, 8+i*2, f); }
            else         { wchar_t e[] = L"□"; mvaddwstr(0, 8+i*2, e); }
        }
        mvprintw(0, 8 + maxHp*2 + 3, "NIVEL %d", currentLevel);

        if (stuffed) { mvprintw(2, 2, "BOCA: "); mvaddwstr(2, 8, L"\u25cf"); mvprintw(2, 10, "(E/S)"); }
        else         { mvprintw(2, 2, "BOCA: "); mvaddwstr(2, 8, L"\u25cb"); }

        const wchar_t** picon = (powerup == POWER_FIRE)  ? ICON_FIRE  :
                                (powerup == POWER_BEAM)  ? ICON_BEAM  :
                                (powerup == POWER_SWORD) ? ICON_SWORD : ICON_NONE;
        int iconLeft = SCREEN_W - ICON_W - 1;
        for (int r = 0; r < ICON_H; r++) {
            if (r >= SCREEN_H) break;
            for (int c = 0; c < ICON_W; c++) {
                int xx = iconLeft + c;
                if (xx < 1 || xx >= SCREEN_W) continue;
                wchar_t cell[2] = { picon[r][c], L'\0' };
                mvaddwstr(r, xx, cell);
            }
        }

        mvprintw(1, 0, "Estado: %s  HP: %d/%d",
            jumping && velY < 0  ? "SUBIENDO " :
            jumping && velY >= 0 ? "CAYENDO  " :
            moving               ? "CAMINANDO" : "IDLE     ",
            hp, maxHp);

        refresh();
        pthread_mutex_unlock(&ncMutex);
        usleep(1000000 / FPS);
    }
    return nullptr;
}
