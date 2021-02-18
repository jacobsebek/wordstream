#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

_Bool font_init();
void font_dealloc();

unsigned render_char_cached(size_t apb_index, const char c, int x, int y, double scale);
void render_string_cached(size_t apb_index, const char* str, int x, int y, double scale);
unsigned cached_string_width(size_t apb_index, const char* str);

SDL_Texture* string_cache(const char* str, SDL_Color col);
void render_string(const char* str, int x, int y, double scale);
