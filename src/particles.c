#include "particles.h"

#include <SDL.h>
#include <stdlib.h>
#include <math.h>

#define RANDOM() ((double)rand()/RAND_MAX)

#define POOLSIZE 256
#define BURSTSIZE 32

struct particle {
    double x, y;
    double vx, vy;
};

static struct particle ppool[POOLSIZE];
static size_t ppool_oldest = 0;

void particles_reset() {
    // There isn't really a way to remove the particles becuse *THEY'RE THERE* at all times
    // All I can do is set the velocity to 0, which makes the particle invisible
    memset(ppool, 0, sizeof(ppool));
    ppool_oldest = 0;
}

void particles_start(int x, int y) {

    for (size_t i = 0; i < BURSTSIZE; i++, ppool_oldest = (ppool_oldest+1) % POOLSIZE) {

        ppool[ppool_oldest].x = (double)(x);
        ppool[ppool_oldest].y = (double)(y);

        double angle = RANDOM() * M_PI * 2;

        ppool[ppool_oldest].vx = cos(angle)*RANDOM()*2;
        ppool[ppool_oldest].vy = sin(angle)*RANDOM()*2;
    }
}

void particles_draw() {

    extern SDL_Renderer* ren;

    for (size_t i = 0; i < POOLSIZE; i++) {

        // Make slower particles darker
        double alpha = fabs(ppool[i].vx)*255.0;
        SDL_SetRenderDrawColor(ren, 100+alpha/2, 200+alpha/5, 255, (int)alpha);

        // Draw the particle
        SDL_RenderFillRect(ren, &(SDL_Rect){(int)ppool[i].x, (int)ppool[i].y, 3, 3});

        // Decrease their velocity a bit
        ppool[i].x += ppool[i].vx*=0.995;
        ppool[i].y += ppool[i].vy*=0.995;
    }
}
