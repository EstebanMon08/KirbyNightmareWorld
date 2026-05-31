#include <locale.h>
#include <ncurses.h>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>

#define FPS         30
#define WORLD_W    1000

/* Tamaño real de la terminal, detectado en runtime */
int SCREEN_W = 80;
int SCREEN_H = 24;

/* ── Sprites con caracteres especiales (wchar_t) ── */

#define KIRBY_H     21
#define KIRBY_W     45

/* Intercambia caracteres espejo al voltear un sprite */
wchar_t mirrorChar(wchar_t c) {
    switch (c) {
        case L'(': return L')';
        case L')': return L'(';
        case L'/': return L'\\';
        case L'\\': return L'/';
        case L'<': return L'>';
        case L'>': return L'<';
        case L'[': return L']';
        case L']': return L'[';
        case L'{': return L'}';
        case L'}': return L'{';
        default:   return c;
    }
}

/* Voltea una línea de sprite horizontalmente */
void flipLine(const wchar_t* src, wchar_t* dst, int maxLen) {
    int len = wcslen(src);
    if (len > maxLen - 1) len = maxLen - 1;
    for (int i = 0; i < len; i++) {
        dst[i] = mirrorChar(src[len - 1 - i]);
    }
    dst[len] = L'\0';
}

const wchar_t* SPRITE_IDLE[KIRBY_H] = {
   L"                                      ",
   L"                                      ",
   L"              ███████████             ",
   L"          ████           ████         ",
   L"       ███                   ███      ",
   L"     ██                         ██    ",
   L"    █                             █   ",
   L"    █                              █  ",
   L"  ██                 ▒▒█   ▒▒█     █  ",
   L" █                   ▒▒█   ▒▒█      █ ",
   L"█                    ███   ███       █",
   L"█                    ███   ███       █",
   L"█            ▒▒▒▒▒             ▒▒▒▒  █",
   L"█                                    █",
   L"█       █               ███       █  █",
   L" ██    █                         ██ █ ",
   L"   ████                         ████  ",
   L"       ███                   █████    ",
   L"    ███▒▒▒███████       █████▒▒▒▒██   ",
   L"  ██▒▒▒▒▒▒▒▒▒▒▒▒████████▒▒▒▒▒▒▒▒▒▒▒██ ",
   L"    █████████████      █████████████  "
};

const wchar_t* SPRITE_WALK[KIRBY_H] = {
   L"                 ███████████                 ",
   L"             ████           ████             ",
   L"          ███                   ███          ",
   L"        ██                         ██        ",
   L"     ██████                          ██      ",
   L"   ███                  ▒▒█   ▒▒█     ████   ",
   L" ███                    ▒▒█   ▒▒█     ██ ██  ",
   L"██                      ███   ███     ██   ██",
   L"██                      ███   ███     ██    █",
   L"██              ▒▒▒▒▒             ▒▒▒▒██    █",
   L" ██                                   ██   ██",
   L"   ███                     ███        ██  ██ ",
   L"     █████                           █████   ",
   L"       ██                           ██       ",
   L"      ████                         ████████  ",
   L"     ██▒▒████                   ████▒▒▒▒▒▒██ ",
   L"     ██▒▒▒▒▒▒███           █████▒▒▒▒▒▒▒▒▒▒██ ",
   L"      ██▒▒▒▒▒▒▒▒██████████████▒▒▒▒▒▒▒▒█████  ",
   L"       ███▒▒▒▒▒▒▒▒██         ██████████      ",
   L"         ████▒▒▒▒███                         ",
   L"            ██████                           ",
};

const wchar_t* SPRITE_JUMP[KIRBY_H] = {
   L"            █████████████            ",
   L"  ███████ ██             ██ ███████  ",
   L" █       █                 █      ██ ",
   L"█                                  ██",
   L"█                                   █",
   L" █                     ▒█  ▒█      ██",
   L"  ██                   ▒█  ▒█     ██ ",
   L"  █                    ██  ██     █  ",
   L" █              ▒▒▒           ▒▒▒  █ ",
   L" █                  █              █ ",
   L" █                   █████████     █ ",
   L" █                  █              █ ",
   L"  █                                █ ",
   L"  █                               █  ",
   L"   ██                            █   ",
   L"  ████                         ██    ",
   L" ██▒▒▒██                      █      ",
   L" █▒▒▒▒▒▒██████           █████       ",
   L" █▒▒▒▒▒▒█▒▒▒█ ███████████            ",
   L" ██▒▒▒██▒▒▒█                         ",
   L"   ███  ███                          ",
};

const wchar_t* SPRITE_FALL[KIRBY_H] = {
   L"            █████████████            ",
   L"  ███████ ██             ██ ███████  ",
   L" █       █                 █      ██ ",
   L"█                                  ██",
   L"█                                   █",
   L" █                     ▒█  ▒█      ██",
   L"  ██                   ▒█  ▒█     ██ ",
   L"  █                    ██  ██     █  ",
   L" █              ▒▒▒           ▒▒▒  █ ",
   L" █                  █              █ ",
   L" █                   █████████     █ ",
   L" █                  █              █ ",
   L"  █                                █ ",
   L"  █                               █  ",
   L"   ██                            █   ",
   L"  ████                         ██    ",
   L" ██▒▒▒██                      █      ",
   L" █▒▒▒▒▒▒██████           █████       ",
   L" █▒▒▒▒▒▒█▒▒▒█ ███████████            ",
   L" ██▒▒▒██▒▒▒█                         ",
   L"   ███  ███                          ",
};

/* ── Enemigo (cubo de prueba) ── */

#define ENEMY_H 5
#define ENEMY_W 8

const wchar_t* SPRITE_ENEMY[ENEMY_H] = {
    L"████████",
    L"█ ▒  ▒ █",
    L"█      █",
    L"█ ▒▒▒▒ █",
    L"████████",
};

struct Enemy {
    float x, y;
    int   w, h;
    bool  alive;
    int   dir;          /* 1 = derecha, -1 = izquierda */
    float speed;        /* velocidad de movimiento */
    int   dirTimer;     /* frames restantes antes de cambiar dirección */
};

const int NUM_ENEMIES = 1;
Enemy enemies[NUM_ENEMIES];

void initEnemies() {
    int groundY = SCREEN_H - 3;
    enemies[0] = { 80, (float)(groundY - ENEMY_H), ENEMY_W, ENEMY_H,
                   true, 1, 1.0f, 150 };  /* dir=derecha, speed=1, 150 frames=5seg */
}

/* ── Plataformas ── */

struct Platform { float x, y; int width; };
const int NUM_PLATFORMS = 8;
Platform platforms[NUM_PLATFORMS];

void initPlatforms() {
    int groundY = SCREEN_H - 3;
    platforms[0] = {  0,  (float)groundY, 180};
    platforms[1] = {200,  (float)groundY, 120};
    platforms[2] = {350,  (float)groundY, 250};
    platforms[3] = { 30,  (float)(groundY - 8),  25};
    platforms[4] = { 90,  (float)(groundY - 15), 20};
    platforms[5] = {160,  (float)(groundY - 20), 18};
    platforms[6] = {260,  (float)(groundY - 16), 22};
    platforms[7] = {380,  (float)(groundY - 10), 30};
}

/* ── Estado del juego ── */

#define INVINCIBLE_FRAMES 60  /* frames de invencibilidad tras recibir daño */

struct GameState {
    float worldX, y, velY, cameraX;
    bool  onGround, jumping, running;
    bool  keyLeft, keyRight, keyJump;
    int   jumpCount;
    bool  jumpHeld;
    int   facingDir;       /* 1 = derecha, -1 = izquierda */
    int   hp, maxHp;
    int   invincibleTimer; /* frames restantes de invencibilidad */
    bool  gameOver;
    int   gameOverTimer;   /* frames restantes antes de reiniciar */
};

/* Mutex para estado del juego */
pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
/* Mutex dedicado para ncurses */
pthread_mutex_t ncMutex = PTHREAD_MUTEX_INITIALIZER;

GameState gState;

static int noLeftCount  = 99;
static int noRightCount = 99;
static int noJumpCount  = 99;
const  int RELEASE_THRESHOLD = 8;

/* Reiniciar el juego a su estado inicial */
void resetGame() {
    gState.worldX = 10; gState.y = platforms[0].y - KIRBY_H;
    gState.velY = 0; gState.cameraX = 0; gState.onGround = true;
    gState.jumping = false;
    gState.keyLeft = false; gState.keyRight = false; gState.keyJump = false;
    gState.jumpCount = 0; gState.jumpHeld = false;
    gState.facingDir = 1;
    gState.hp = gState.maxHp;
    gState.invincibleTimer = 0;
    gState.gameOver = false;
    gState.gameOverTimer = 0;
    /* Reiniciar enemigos */
    initEnemies();
}

/* ── Hilo de input ── */

void* inputThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }
        pthread_mutex_unlock(&gMutex);

        int ch;
        bool sawLeft = false, sawRight = false, sawJump = false;

        pthread_mutex_lock(&ncMutex);
        while ((ch = getch()) != ERR) {
            switch (ch) {
                case 'a': case 'A': sawLeft  = true; break;
                case 'd': case 'D': sawRight = true; break;
                case 'w': case 'W': case ' ': sawJump = true; break;
                case 'q': case 'Q': case 27:
                    pthread_mutex_unlock(&ncMutex);
                    pthread_mutex_lock(&gMutex);
                    gState.running = false;
                    pthread_mutex_unlock(&gMutex);
                    return nullptr;
            }
        }
        pthread_mutex_unlock(&ncMutex);

        if (sawLeft)  noLeftCount  = 0; else noLeftCount++;
        if (sawRight) noRightCount = 0; else noRightCount++;
        if (sawJump)  noJumpCount  = 0; else noJumpCount++;

        pthread_mutex_lock(&gMutex);
        gState.keyLeft  = (noLeftCount  < RELEASE_THRESHOLD);
        gState.keyRight = (noRightCount < RELEASE_THRESHOLD);
        gState.keyJump  = (noJumpCount  < RELEASE_THRESHOLD);
        pthread_mutex_unlock(&gMutex);

        usleep(8000);
    }
    return nullptr;
}

/* ── Hilo de fisica ── */

void* physicsThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }

        /* Si estamos en game over, solo contar el timer y reiniciar */
        if (gState.gameOver) {
            gState.gameOverTimer--;
            if (gState.gameOverTimer <= 0) {
                resetGame();
            }
            pthread_mutex_unlock(&gMutex);
            usleep(33333);
            continue;
        }

        /* Movimiento */
        if (gState.keyLeft)  { gState.worldX -= 2.0f; gState.facingDir = -1; }
        if (gState.keyRight) { gState.worldX += 2.0f; gState.facingDir =  1; }
        if (gState.worldX < 0) gState.worldX = 0;
        if (gState.worldX > WORLD_W - KIRBY_W) gState.worldX = WORLD_W - KIRBY_W;

        /* Salto */
        if (gState.keyJump && !gState.jumpHeld) {
            if (gState.onGround) {
                gState.velY = -3.0f; gState.onGround = false;
                gState.jumping = true; gState.jumpCount = 1;
            } else if (gState.jumpCount < 5) {
                gState.jumpCount++;
                switch (gState.jumpCount) {
                    case 2: gState.velY = -2.0f; break;
                    case 3: gState.velY = -2.0f; break;
                    case 4: gState.velY = -2.0f; break;
                    case 5: gState.velY = -2.0f; break;
                }
            }
            gState.jumpHeld = true;
        }
        if (!gState.keyJump) gState.jumpHeld = false;

        /* Gravedad */
        if (!gState.onGround) { gState.velY += 0.18f; gState.y += gState.velY; }

        /* Respawn si cae fuera de pantalla */
        if (gState.y > SCREEN_H + 5) {
            gState.y = platforms[0].y - KIRBY_H;
            gState.velY = 0; gState.onGround = true;
            gState.jumping = false; gState.jumpCount = 0;
        }

        /* Colisión con plataformas */
        gState.onGround = false;
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            Platform& p = platforms[i];
            bool dentroX = (gState.worldX + KIRBY_W > p.x) && (gState.worldX < p.x + p.width);
            bool cayendo = gState.velY >= 0;
            bool sobreY  = (gState.y + KIRBY_H >= p.y) && (gState.y + KIRBY_H <= p.y + 3);
            if (dentroX && cayendo && sobreY) {
                gState.y = p.y - KIRBY_H; gState.velY = 0;
                gState.onGround = true; gState.jumping = false; gState.jumpCount = 0;
                break;
            }
        }

        /* Movimiento de enemigos */
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];
            e.x += e.speed * e.dir;
            e.dirTimer--;
            if (e.dirTimer <= 0) {
                e.dir *= -1;       /* cambiar dirección */
                e.dirTimer = 150;  /* 150 frames ≈ 5 segundos */
            }
        }

        /* Reducir timer de invencibilidad */
        if (gState.invincibleTimer > 0) gState.invincibleTimer--;

        /* Colisión con enemigos */
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];
            /* Hitbox: centro de Kirby vs cubo */
            float kx1 = gState.worldX + 5;   /* margen interno para hitbox */
            float ky1 = gState.y + 3;
            float kx2 = gState.worldX + KIRBY_W - 5;
            float ky2 = gState.y + KIRBY_H - 3;
            float ex1 = e.x;
            float ey1 = e.y;
            float ex2 = e.x + e.w;
            float ey2 = e.y + e.h;

            bool colision = (kx1 < ex2) && (kx2 > ex1) && (ky1 < ey2) && (ky2 > ey1);

            if (colision && gState.invincibleTimer == 0) {
                gState.hp--;
                gState.invincibleTimer = INVINCIBLE_FRAMES;
                /* Knockback: empujar a Kirby en dirección opuesta */
                if (gState.worldX < e.x) {
                    gState.worldX -= 10.0f;
                } else {
                    gState.worldX += 10.0f;
                }
                gState.velY = -2.0f;
                gState.onGround = false;
                gState.jumping = true;

                /* Game over si no queda vida */
                if (gState.hp <= 0) {
                    gState.hp = 0;
                    gState.gameOver = true;
                    gState.gameOverTimer = 90; /* 90 frames ≈ 3 segundos */
                }
            }
        }

        /* Cámara */
        float targetCam = gState.worldX - SCREEN_W / 2.0f;
        if (targetCam < 0) targetCam = 0;
        if (targetCam > WORLD_W - SCREEN_W) targetCam = WORLD_W - SCREEN_W;
        gState.cameraX += (targetCam - gState.cameraX) * 0.2f;

        pthread_mutex_unlock(&gMutex);
        usleep(33333);
    }
    return nullptr;
}

/* ── Hilo de render ── */

void* renderThread(void*) {
    while (true) {
        /* Leer estado del juego */
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }
        float camX    = gState.cameraX;
        float kWorldX = gState.worldX;
        float ky      = gState.y;
        float velY    = gState.velY;
        bool  jumping = gState.jumping;
        bool  moving  = gState.keyLeft || gState.keyRight;
        int   jumps   = gState.jumpCount;
        int   facing  = gState.facingDir;
        int   hp      = gState.hp;
        int   maxHp   = gState.maxHp;
        int   invTimer = gState.invincibleTimer;
        bool  isGameOver = gState.gameOver;
        pthread_mutex_unlock(&gMutex);

        /* Proteger TODO el dibujado con ncMutex */
        pthread_mutex_lock(&ncMutex);

        erase();

        /* Pantalla de game over */
        if (isGameOver) {
            mvprintw(SCREEN_H / 2, SCREEN_W / 2 - 5, "GAME  OVER");
            refresh();
            pthread_mutex_unlock(&ncMutex);
            usleep(1000000 / FPS);
            continue;
        }

        /* Dibujar plataformas */
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            Platform& p = platforms[i];
            int sx = (int)(p.x - camX);
            for (int j = 0; j < p.width; j++) {
                int px = sx + j;
                if (px >= 0 && px < SCREEN_W && (int)p.y >= 2 && (int)p.y < SCREEN_H) {
                    wchar_t blk[] = L"═";
                    mvaddwstr((int)p.y, px, blk);
                }
            }
        }

        /* Dibujar enemigos */
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];
            int ex = (int)(e.x - camX);
            for (int r = 0; r < e.h; r++) {
                int row = (int)e.y + r;
                if (row >= 2 && row < SCREEN_H && ex >= 0 && ex < SCREEN_W)
                    mvaddwstr(row, ex, SPRITE_ENEMY[r]);
            }
        }

        /* Seleccionar sprite de Kirby */
        const wchar_t** sprite;
        if      (jumping && velY <  0) sprite = SPRITE_JUMP;
        else if (jumping && velY >= 0) sprite = SPRITE_FALL;
        else if (moving)               sprite = SPRITE_WALK;
        else                           sprite = SPRITE_IDLE;

        /* Dibujar Kirby (voltear si mira a la izquierda) */
        /* Parpadeo durante invencibilidad */
        bool visible = (invTimer == 0) || ((invTimer / 4) % 2 == 0);
        int kx = (int)(kWorldX - camX);
        wchar_t flipped[KIRBY_W + 10];
        if (visible) {
            for (int r = 0; r < KIRBY_H; r++) {
                int row = (int)ky + r;
                if (row >= 2 && row < SCREEN_H && kx >= 0 && kx < SCREEN_W) {
                    if (facing == -1) {
                        flipLine(sprite[r], flipped, KIRBY_W + 10);
                        mvaddwstr(row, kx, flipped);
                    } else {
                        mvaddwstr(row, kx, sprite[r]);
                    }
                }
            }
        }

        /* HUD - Vida de Kirby */
        mvprintw(0, 2, "KIRBY");
        for (int i = 0; i < maxHp; i++) {
            if (i < hp) {
                wchar_t full[] = L"■";
                mvaddwstr(0, 8 + i * 2, full);
            } else {
                wchar_t empty[] = L"□";
                mvaddwstr(0, 8 + i * 2, empty);
            }
        }

        /* Estado (debug) */
        mvprintw(1, 0, "Estado: %s  HP: %d/%d",
            jumping && velY <  0 ? "SUBIENDO " :
            jumping && velY >= 0 ? "CAYENDO  " :
            moving               ? "CAMINANDO" : "IDLE     ",
            hp, maxHp);

        refresh();

        pthread_mutex_unlock(&ncMutex);

        usleep(1000000 / FPS);
    }
    return nullptr;
}

/* ── Main ── */

int main() {
    setlocale(LC_ALL, "");
    initscr(); cbreak(); noecho();
    curs_set(0); nodelay(stdscr, TRUE); keypad(stdscr, TRUE);

    /* Detectar tamaño real de la terminal */
    getmaxyx(stdscr, SCREEN_H, SCREEN_W);

    /* Inicializar mundo */
    initPlatforms();
    initEnemies();

    gState.worldX = 10; gState.y = platforms[0].y - KIRBY_H;
    gState.velY = 0; gState.cameraX = 0; gState.onGround = true;
    gState.jumping = false; gState.running = true;
    gState.keyLeft = false; gState.keyRight = false; gState.keyJump = false;
    gState.jumpCount = 0; gState.jumpHeld = false;
    gState.facingDir = 1;
    gState.hp = 6; gState.maxHp = 6;
    gState.invincibleTimer = 0;
    gState.gameOver = false;
    gState.gameOverTimer = 0;

    pthread_t tIn, tPh, tRe;
    pthread_create(&tIn, nullptr, inputThread,   nullptr);
    pthread_create(&tPh, nullptr, physicsThread, nullptr);
    pthread_create(&tRe, nullptr, renderThread,  nullptr);
    pthread_join(tIn, nullptr);
    pthread_join(tPh, nullptr);
    pthread_join(tRe, nullptr);

    pthread_mutex_destroy(&gMutex);
    pthread_mutex_destroy(&ncMutex);
    endwin();
    return 0;
}