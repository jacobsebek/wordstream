#include <SDL.h>
#include <SDL_mixer.h>

#include <stdio.h> // stderr, fprintf

#include "text.h"
#include "game.h"

SDL_Window* win;
SDL_Renderer* ren;
const int WIDTH = 640, HEIGHT = 360, BARHEIGHT = 50;

static SDL_Texture* load_texture(const char* filename) {
    SDL_Surface* surf = SDL_LoadBMP(filename);
    if (!surf) return NULL;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    return tex;
}

// Render texture with the anchor point in the middle instead of the top left
static void render_middle(SDL_Texture* tex, int x, int y, double scale) {
    SDL_Rect dstr;
    if (SDL_QueryTexture(tex, NULL, NULL, &dstr.w, &dstr.h))
        return;

    dstr.w = (int)(dstr.w*scale);
    dstr.h = (int)(dstr.h*scale);

    dstr.x = x-dstr.w/2;
    dstr.y = y-dstr.h/2;

    SDL_RenderCopy(ren, tex, NULL, &dstr);
}

static void set_icon(SDL_Surface* icon) {
    if (!icon) return;

    // Convert the surface to a 32bit bitmap
    SDL_Surface* new_s = SDL_ConvertSurfaceFormat(icon, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!new_s) 
        goto quit;

    SDL_FreeSurface(icon);
    icon = new_s;
    
    // Make green pixels fully transparent (BMP doesn't support transparency)
    if (SDL_LockSurface(icon)) 
        goto quit;

    for (int i = 0; i < icon->w * icon->h; i++) {
        Uint32* pixel = (Uint32*)( (char*)icon->pixels+i*4 );
        Uint8 r, g, b, a;
        SDL_GetRGBA(*pixel, icon->format, &r, &g, &b, &a);
        if (r == 0 && g == 255 && b == 0 && a == 255) 
            *pixel = SDL_MapRGBA(icon->format, 0, 0, 0, 0);
    }
    SDL_UnlockSurface(icon);

    SDL_SetWindowIcon(win, icon);

    quit:
        SDL_FreeSurface(icon);
}

int main(int argc, char *argv[]) {

    // The compiler complains about us not using the variables
    (void)argc; (void)argv;

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL2 failed to initialize: %s\n", SDL_GetError());
        exit(1);
    }

    // Create the window with the renderer
    if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_SHOWN, &win, &ren)) {
        fprintf(stderr, "Failed to create a window: %s\n", SDL_GetError());
        exit(1);
    }

    // Set the transparent blend mode
    if (SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND)) {
        fprintf(stderr, "Failed to set the blend mode: %s\n", SDL_GetError());
        exit(1);
    }

    // Set the title
    SDL_SetWindowTitle(win, "Wordstream - A typing game");
    
    // Set the icon
    set_icon(SDL_LoadBMP("res/icon.bmp"));

    // Initialize SDL_Mixer
    // Notice that we don't quit when it fails, it is optional
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 1, 4096))
        fprintf(stderr, "SDL_Mixer failed to initialize : %s\n", Mix_GetError());

    // Initialise the game and fonts, fonts rather first
    if (!font_init() | !game_init()) exit(1);

    // Cache some textures
    SDL_Texture* start_tex, *lost_tex; 
    SDL_Texture* bg;

    // This will be used to display the scores on the losing screen
    SDL_Texture* lost_info_tex[NUM_SCORES];

    // Cache the starting screen
    start_tex = string_cache("Press SPACE to play", (SDL_Color){200, 200, 255, 255});

    // Cache the losing screen
    lost_tex = string_cache("You have lost, try again!",(SDL_Color){200, 200, 255, 255});

    // Load the background image
    bg = load_texture("res/bg.bmp");    

    SDL_StopTextInput();
    enum { STATE_START, STATE_GAME, STATE_LOST, STATE_QUIT } state = STATE_START;
    while (1) {

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT : 
                    state = STATE_QUIT;
                break;
                case SDL_KEYDOWN : 
                    switch(e.key.keysym.sym) {
                        case SDLK_SPACE :
                            // START THE GAME !
                            if (state != STATE_GAME) {
                                state = STATE_GAME;

                                game_start();
                                SDL_StartTextInput();
                            }
    
                        break;
                        case SDLK_BACKSPACE : {
                            game_input_delete(1);
                        } break;
                    }
                break;
                case SDL_TEXTINPUT :
                    game_textinput(e.text.text);
                break;
            }
        }

        if (state == STATE_QUIT) break;

        // Clear the background
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        // Render the scrolling background texture
        SDL_RenderCopy(ren, bg, NULL, &(SDL_Rect){((SDL_GetTicks()/100) % WIDTH), 0, WIDTH, HEIGHT});
        SDL_RenderCopy(ren, bg, NULL, &(SDL_Rect){((SDL_GetTicks()/100) % WIDTH - WIDTH), 0, WIDTH, HEIGHT});

        // If we are at the starting or ending screen, draw this dark rectangle
        if (state != STATE_GAME) {
            SDL_SetRenderDrawColor(ren, 0,0,0,200);
            SDL_RenderFillRect(ren, &(SDL_Rect){WIDTH/2-200, 0, 400, HEIGHT});
        }

        switch (state) {
            case STATE_START :
                // Just draw "press spacebar to play"
                render_middle(start_tex, WIDTH/2, HEIGHT/2, 1.0);
            break;
            case STATE_GAME : 

                // game_draw returns false if we lost
                if (!game_draw()) {
                    state = STATE_LOST; 

                    // Destroy the textures before rewriting
                    for (size_t i = 0; i < NUM_SCORES; i++)
                        SDL_DestroyTexture(lost_info_tex[i]);
                    // Render the scores to the texture
                    game_render_scores(lost_info_tex);

                    SDL_StopTextInput();
                }
            break;
            case STATE_LOST :

                // Just draw the "you lost"...
                render_middle(lost_tex, WIDTH/2, 50, 0.8);

                // Draw all the scores
                for (size_t i = 0; i < NUM_SCORES; i++)
                    if (lost_info_tex[i] != NULL) {
                        int w, h;
                        SDL_QueryTexture(lost_info_tex[i], NULL, NULL, &w, &h);    
                        SDL_RenderCopy(ren, lost_info_tex[i], NULL, &(SDL_Rect){WIDTH/2-180, 20+65+15*i, w/2, h/2});    
                    }

            break;

            default:
            break;
        }    

        SDL_RenderPresent(ren);

        //TODO: a better loop
        SDL_Delay(10);

    }

    // The exit takes a while, the window would be lagged and not responding
    SDL_HideWindow(win);
    
    SDL_DestroyTexture(bg);
    SDL_DestroyTexture(lost_tex);
    SDL_DestroyTexture(start_tex);

    for (size_t i = 0; i < 5; i++)
        if (lost_info_tex[i] != NULL) 
            SDL_DestroyTexture(lost_info_tex[i]);

    font_dealloc();
    game_dealloc();

    Mix_CloseAudio();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
