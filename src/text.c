#include "text.h"

#include <stdio.h>

extern SDL_Renderer* ren;

static TTF_Font* font;
static SDL_Texture* alphabet[3][26];

static _Bool alphabet_cache(size_t index, SDL_Color col) {

    for (size_t i = 0; i < 26; i++) {

        SDL_Surface* surf = TTF_RenderGlyph_Solid(font, 'a'+i, col);
        if (!surf) return 0;

        alphabet[index][i] = SDL_CreateTextureFromSurface(ren, surf);
        if (!alphabet[index][i]) return 0;
        SDL_FreeSurface(surf);
    }

    return 1;
}

static void alphabet_destroy(size_t index) {
    for (size_t i = 0; i < 26; i++)
        SDL_DestroyTexture(alphabet[index][i]);
}


_Bool font_init() {

    // Initialize the font
    if (TTF_Init()) {
        fprintf(stderr, "SDL_ttf failed to initialize : %s\n", TTF_GetError());
        return 0;
    }

    font = TTF_OpenFont("res/font.ttf", 32);
    if (font == NULL) {
        fprintf(stderr, "Failed to load the font file: %s\n", TTF_GetError());
        return 0;
    }
    
    // Cache the three different colors of alphabets
    if (!alphabet_cache(0, (SDL_Color){100, 200, 255, 255}) |
        !alphabet_cache(1, (SDL_Color){245, 245, 255, 255}) |
        !alphabet_cache(2, (SDL_Color){255, 255, 255, 255}))  {
        fprintf(stderr, "Failed to cache alphabets\n");
        return 0;
    }

    return 1;
}

void font_dealloc() {
    alphabet_destroy(0);
    alphabet_destroy(1);
    alphabet_destroy(2);

    TTF_CloseFont(font);
    TTF_Quit();
}

unsigned render_char_cached(size_t apb_index, const char c, int x, int y, double scale) {
    SDL_Texture* glyph = alphabet[apb_index][c-'a']; 
    
    SDL_Rect dstr;
    dstr.x = x;
    dstr.y = y;
    if (SDL_QueryTexture(glyph, NULL, NULL, &dstr.w, &dstr.h)) 
        return 0;

    dstr.w = (int)(dstr.w*scale);
    dstr.h = (int)(dstr.h*scale);

    SDL_RenderCopy(ren, glyph, NULL, &dstr);

    return dstr.w;
}

void render_string_cached(size_t apb_index, const char* str, int x, int y, double scale) {
    unsigned offset = 0;
    for (; *str; str++)
        offset += render_char_cached(apb_index, *str, x+offset, y, scale);
}

unsigned cached_string_width(size_t apb_index, const char* str) {
    unsigned sum = 0;

    for (; *str; str++) {
        int wid;
        SDL_QueryTexture(alphabet[apb_index][*str-'a'], NULL, NULL, &wid, NULL);
        sum += wid;
    }

    return sum;
}

SDL_Texture* string_cache(const char* str, SDL_Color col) {
    SDL_Surface* surf = TTF_RenderText_Solid(font, str, col);
    if (!surf) return NULL;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    return tex;
}

void render_string(const char* str, int x, int y, double scale) {

    SDL_Surface* surf = TTF_RenderText_Solid(font, str, (SDL_Color){255,255,255,255});
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    if (!tex) return;

    SDL_Rect dstr = {x, y, surf->w, surf->h};

    dstr.w = (int)(dstr.w*scale);
    dstr.h = (int)(dstr.h*scale);

    SDL_RenderCopy(ren, tex, NULL, &dstr);

    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);

}
