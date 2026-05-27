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

/* ── Plataformas ── */

struct Platform { float x, y; int width; };
const int NUM_PLATFORMS = 8;
Platform platforms[NUM_PLATFORMS];

/* Posicionar plataformas relativo al tamaño real de la terminal */
void initPlatforms() {
    int groundY = SCREEN_H - 3;  /* suelo cerca del fondo */
    /* Suelo base */
    platforms[0] = {  0,  (float)groundY, 180};
    platforms[1] = {200,  (float)groundY, 120};
    platforms[2] = {350,  (float)groundY, 250};
    /* Plataformas elevadas (relativas al suelo) */
    platforms[3] = { 30,  (float)(groundY - 8),  25};
    platforms[4] = { 90,  (float)(groundY - 15), 20};
    platforms[5] = {160,  (float)(groundY - 20), 18};
    platforms[6] = {260,  (float)(groundY - 16), 22};
    platforms[7] = {380,  (float)(groundY - 10), 30};
}

/* ── Estado del juego ── */

struct GameState {
    float worldX, y, velY, cameraX;
    bool  onGround, jumping, running;
    bool  keyLeft, keyRight, keyJump;
    int   jumpCount;
    bool  jumpHeld;
};

/* Mutex para estado del juego */
pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
/* Mutex dedicado para ncurses (getch, erase, mvaddch, refresh, etc.) */
pthread_mutex_t ncMutex = PTHREAD_MUTEX_INITIALIZER;

GameState gState;

static int noLeftCount  = 99;
static int noRightCount = 99;
static int noJumpCount  = 99;
const  int RELEASE_THRESHOLD = 8;

/* ── Hilo de input ── */

void* inputThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }
        pthread_mutex_unlock(&gMutex);

        int ch;
        bool sawLeft = false, sawRight = false, sawJump = false;

        /* Proteger getch() con ncMutex */
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

        if (gState.keyLeft)  gState.worldX -= 2.0f;
        if (gState.keyRight) gState.worldX += 2.0f;
        if (gState.worldX < 0) gState.worldX = 0;
        if (gState.worldX > WORLD_W - KIRBY_W) gState.worldX = WORLD_W - KIRBY_W;

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

        if (!gState.onGround) { gState.velY += 0.18f; gState.y += gState.velY; }

        if (gState.y > SCREEN_H + 5) {
            gState.y = platforms[0].y - KIRBY_H;
            gState.velY = 0; gState.onGround = true;
            gState.jumping = false; gState.jumpCount = 0;
        }

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
        pthread_mutex_unlock(&gMutex);

        /* Proteger TODO el dibujado con ncMutex */
        pthread_mutex_lock(&ncMutex);

        erase();

        /* Dibujar plataformas con '═' */
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

        /* Seleccionar sprite */
        const wchar_t** sprite;
        if      (jumping && velY <  0) sprite = SPRITE_JUMP;
        else if (jumping && velY >= 0) sprite = SPRITE_FALL;
        else if (moving)               sprite = SPRITE_WALK;
        else                           sprite = SPRITE_IDLE;

        /* Dibujar Kirby */
        int kx = (int)(kWorldX - camX);
        for (int r = 0; r < KIRBY_H; r++) {
            int row = (int)ky + r;
            if (row >= 2 && row < SCREEN_H && kx >= 0 && kx < SCREEN_W)
                mvaddwstr(row, kx, sprite[r]);
        }

        /* HUD */
        mvprintw(1, 0, "Estado: %s",
            jumping && velY <  0 ? "SUBIENDO " :
            jumping && velY >= 0 ? "CAYENDO  " :
            moving               ? "CAMINANDO" : "IDLE     ");

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

    /* Posicionar plataformas relativo al tamaño detectado */
    initPlatforms();

    gState.worldX = 10; gState.y = platforms[0].y - KIRBY_H;
    gState.velY = 0; gState.cameraX = 0; gState.onGround = true;
    gState.jumping = false; gState.running = true;
    gState.keyLeft = false; gState.keyRight = false; gState.keyJump = false;
    gState.jumpCount = 0; gState.jumpHeld = false;

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