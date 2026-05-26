#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>W

#define SCREEN_W   120
#define SCREEN_H   80
#define GROUND_Y    20
#define FPS         30
#define WORLD_W    200
#define KIRBY_H     5
#define KIRBY_W     11




const char* SPRITE_IDLE[KIRBY_H] = {
    "   OOOOO   ",
    " OOOOOOOOO ",
    "OOOOOOOOOOO",
    " OOOOOOOOO ",
    "   OOOOO   "
};

const char* SPRITE_WALK[KIRBY_H] = {
    "   OOOOO   ",
    " OOOOOOOOO ",
    "OOOOOOOOOOO",
    " OOOOOOOOO ",
    "   OOOOO   "
};
const char* SPRITE_JUMP[KIRBY_H] = {
    "   OOOOO   ",
    " OOOOOOOOO ",
    "OOOOOOOOOOO",
    " OOOOOOOOO ",
    "   OOOOO   "
};
const char* SPRITE_FALL[KIRBY_H] = {
    "   OOOOO   ",
    " OOOOOOOOO ",
    "OOOOOOOOOOO",
    " OOOOOOOOO ",
    "   OOOOO   "
};

struct Platform { float x, y; int width; };
const int NUM_PLATFORMS = 8;
Platform platforms[NUM_PLATFORMS] = {
    {  0, 30, 120}, {130, 30,  80}, {220, 30, 180},
    { 25, 23,  15}, { 60, 18,  12}, {100, 22,  18},
    {160, 16,  10}, {220, 20,  20},
};

struct GameState {
    float worldX, y, velY, cameraX;
    bool  onGround, jumping, running;
    bool  keyLeft, keyRight, keyJump;
    int   jumpCount;
    bool  jumpHeld;
};

pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
GameState gState;

static int noLeftCount  = 99;
static int noRightCount = 99;
static int noJumpCount  = 99;
const  int RELEASE_THRESHOLD = 8;

void* inputThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }
        pthread_mutex_unlock(&gMutex);

        int ch;
        bool sawLeft = false, sawRight = false, sawJump = false;
        while ((ch = getch()) != ERR) {
            switch (ch) {
                case 'a': case 'A': sawLeft  = true; break;
                case 'd': case 'D': sawRight = true; break;
                case 'w': case 'W': case ' ': sawJump = true; break;
                case 'q': case 'Q': case 27:
                    pthread_mutex_lock(&gMutex);
                    gState.running = false;
                    pthread_mutex_unlock(&gMutex);
                    return nullptr;
            }
        }

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
                gState.velY = -2.2f; gState.onGround = false;
                gState.jumping = true; gState.jumpCount = 1;
            } else if (gState.jumpCount < 5) {
                gState.jumpCount++;
                switch (gState.jumpCount) {
                    case 2: gState.velY = -2.0f; break;
                    case 3: gState.velY = -1.8f; break;
                    case 4: gState.velY = -1.5f; break;
                    case 5: gState.velY = -0.3f; break;
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

void* renderThread(void*) {
    while (true) {
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

        erase();

        for (int i = 0; i < NUM_PLATFORMS; i++) {
            Platform& p = platforms[i];
            int sx = (int)(p.x - camX);
            for (int j = 0; j < p.width; j++) {
                int px = sx + j;
                if (px >= 0 && px < SCREEN_W && (int)p.y >= 2 && (int)p.y < SCREEN_H)
                    mvaddch((int)p.y, px, '=');
            }
        }

        const char** sprite;
        if      (jumping && velY <  0) sprite = SPRITE_JUMP;
        else if (jumping && velY >= 0) sprite = SPRITE_FALL;
        else if (moving)               sprite = SPRITE_WALK;
        else                           sprite = SPRITE_IDLE;

        int kx = (int)(kWorldX - camX);
        for (int r = 0; r < KIRBY_H; r++) {
            int row = (int)ky + r;
            if (row >= 2 && row < SCREEN_H && kx >= 0 && kx < SCREEN_W)
                mvprintw(row, kx, "%s", sprite[r]);
        }

        mvprintw(1, 0, "Estado: %s",
            jumping && velY <  0 ? "SUBIENDO " :
            jumping && velY >= 0 ? "CAYENDO  " :
            moving              ? "CAMINANDO" : "IDLE     ");

        refresh();
        usleep(1000000 / FPS);
    }
    return nullptr;
}

int main() {
    setlocale(LC_ALL, "");
    initscr(); cbreak(); noecho();
    curs_set(0); nodelay(stdscr, TRUE); keypad(stdscr, TRUE);

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
    endwin();
    return 0;
}