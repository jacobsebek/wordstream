#pragma once

#include <SDL.h>

#define NUM_SCORES 6

_Bool game_init();
void game_dealloc();

void game_start();
_Bool game_draw();

void game_textinput(const char* str);
void game_input_delete(size_t num);

void game_render_scores(SDL_Texture* rows[NUM_SCORES]);
