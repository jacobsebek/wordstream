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
static size_t ppool_len = 0;

void particles_reset() {
    ppool_len = 0;
}

//TODO: am I stupid or does this just replace the old particles SOMETIMES?
void particles_start(int x, int y) {

    // Possibly shift the old particles to make space for new ones
    //TODO: this could be done in a more elaborate way by discarding particles from the oldest
    ppool_len = (ppool_len <= (POOLSIZE - BURSTSIZE) ? ppool_len : (POOLSIZE - BURSTSIZE) );

    for (size_t i = 0; i < BURSTSIZE; i++, ppool_len++) {

        ppool[ppool_len].x = (double)(x);
        ppool[ppool_len].y = (double)(y);

        double angle = RANDOM() * M_PI * 2;

        ppool[ppool_len].vx = cos(angle)*RANDOM()*2;
        ppool[ppool_len].vy = sin(angle)*RANDOM()*2;
    }
}

void particles_draw() {

    extern SDL_Renderer* ren;

    for (size_t i = 0; i < ppool_len; i++) {

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
