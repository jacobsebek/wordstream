#include "game.h"
#include "dict.h"
#include "particles.h"
#include "text.h"

#include <ctype.h> // isspace
#include <stddef.h> // size_t
#include <string.h> // memmove
#include <stdio.h> // sprintf
#include <stdlib.h> // rand

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#define    WORDS 16

extern SDL_Renderer* ren;

extern const int WIDTH, HEIGHT, BARHEIGHT;

// Text stuff
size_t dict_size;
char** dict;
_Bool* dict_used_words;

struct {
    size_t index; // index in the dict array
    double x, y;    
} word_arr[WORDS];

char input_str[WORDLEN];

// The speed of the word stream
double scroll_speed = 0.3;

// Scores
unsigned cpm = 0; // the chars per minute (in the last minute)
unsigned cpm_best = 0; // the overall best cpm
unsigned backspaces = 0; // the number of input character deletions
unsigned words = 0, chars = 0; // Total characters and words typed in this round
unsigned round_start = 0; // When the current round started

// Some sound effects
Mix_Chunk* sound_start;
Mix_Chunk* sound_pop;
Mix_Chunk* sound_end;

// Stores the number of characters typed in the corresponding second one minute ago
// Used for dynamically updating CPM
unsigned chars_in_second[60];

_Bool game_init() {
    // Load the dictionary
    FILE* dictf = fopen("res/dict.txt", "r");
    dict = dict_load(dictf, &dict_size);
    fclose(dictf);

    if (!dict)  {
        fprintf(stderr, "Failed to load the dictionary\n");
        return 0;
    }

    fprintf(stdout, "A total of %zu words has been loaded\n", dict_size);

    // Allocate the used_words array
    dict_used_words = calloc(dict_size, sizeof(_Bool));
    if (!dict_used_words) return 0;

    // Load the sfx
    sound_start = Mix_LoadWAV("res/start.wav");
    sound_pop = Mix_LoadWAV("res/pop.wav");
    sound_end = Mix_LoadWAV("res/end.wav");

    return 1;
}

void game_dealloc() {
    dict_destroy(dict, dict_size);
    free(dict_used_words);

    Mix_FreeChunk(sound_start);
    Mix_FreeChunk(sound_pop);
    Mix_FreeChunk(sound_end);
}

// Pick an unused word from the dictionary
size_t dict_pick() {
    // TODO: we can only index RAND_MAX words

    size_t index = rand() % dict_size;

    for (size_t count = 0; dict_used_words[index]; count++) {

        // If there are no unused words, we have no other choice than to return an already used one
        if (count >= dict_size) 
            return index;

        index++; 
        index %= dict_size;
    }

    return index;
}

void game_start() {

    // Initialise the random generator with a somewhat-random seed
    srand(SDL_GetTicks());

    // Initialize the scores
    words = chars = 0;
    cpm = 0;
    cpm_best = 0;
    backspaces = 0;
    round_start = SDL_GetTicks();

    // Clear the char_in_second table
    memset(chars_in_second, 0, 60 * sizeof(chars_in_second[0]));
    // Mark all words in the dictionary as unused
    memset(dict_used_words, 0, dict_size * sizeof(dict_used_words[0]));

    // Initialize the word stream
    for (size_t i = 0; i < WORDS; i++) {
        word_arr[i].index = dict_pick();
        word_arr[i].x = 0 - rand() % WIDTH - (int)cached_string_width(1, dict[word_arr[i].index]);
        word_arr[i].y = (int)((double)(HEIGHT-BARHEIGHT)/WORDS * (double) i);
    }

    // Reset the particles
    particles_reset();
    
    // Initailise the input string
    input_str[0] = '\0';

    Mix_PlayChannel(-1, sound_start, 0);
}

//TODO: it is not really nice when you type a word that is on the screen multiple times
// maybe it should remove all of them, or the one that is the closest to the right?
void game_textinput(const char* str) {
    // Only write down alphabetical characters
    for (const char* c = str; *c && strlen(input_str) < WORDLEN-1; c++)
        if (!isalpha(*c)) continue;
        else {
            strcat(input_str, (char[]){*c, '\0'});
        }

    // Check if the text matches any word in the word stream
    for (size_t i = 0; i < WORDS; i++) {
        const char* word = dict[word_arr[i].index];

        // If we accidentally write a word that cannot even be seen, ignore it
        if (word_arr[i].x  < 0)
            continue;

        if (!strcmp(word, input_str)) {

            input_str[0] = '\0'; // Clear the input string

            // Add particles for the animation
            particles_start(word_arr[i].x, word_arr[i].y);

            // The word is not used anymore
            dict_used_words[word_arr[i].index] = 0;

            // pick a new word, note that this allows picking the same word again
            word_arr[i].index = dict_pick();
            word_arr[i].x = 0 - rand() % WIDTH - (int)cached_string_width(1, dict[word_arr[i].index]);

            // Increment the scores
            size_t len = strlen(word);

            chars_in_second[(SDL_GetTicks()/1000) % 60] += len;

            chars += len;
            cpm += len;
            words++;

            if (cpm > cpm_best) cpm_best = cpm;

            Mix_PlayChannel(-1, sound_pop, 0);

            break;
        }
    }
}

void game_input_delete(size_t num) {
    size_t input_str_len = strlen(input_str);
    if (input_str_len > 0) {
        input_str[input_str_len-num] = '\0';
        backspaces+=num;
    }
}

_Bool game_draw() {

    for (size_t i = 0, input_str_len = strlen(input_str); i < WORDS; i++) {

        word_arr[i].x += scroll_speed;

        // If one of the words gets too far right, we lose
        if (word_arr[i].x > WIDTH) {
            Mix_PlayChannel(-1, sound_end, 0);
            return 0;
        }

        // DRAWING

        const char* word = dict[word_arr[i].index];
        // This checks wheter the word should be highlited when typing it
        _Bool mismatch = 0;
        for (size_t c = 0; word[c] && input_str[c] && !(mismatch = (word[c] != input_str[c])); c++);
        
        // Draw each letter
        unsigned offset = 0;
        for (size_t c = 0; word[c]; c++)
            offset += render_char_cached(!mismatch && c < input_str_len, word[c], (int)word_arr[i].x+offset, (int)word_arr[i].y, 0.5);

    }

    // Adjust the scrolling speed
    scroll_speed += 0.00001;

    // Draw particles
    particles_draw(ren);

    // Draw the GUI
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderFillRect(ren, &(SDL_Rect){0, HEIGHT-BARHEIGHT, WIDTH, 3});

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 100);
    SDL_RenderFillRect(ren, &(SDL_Rect){0, HEIGHT-BARHEIGHT+3, WIDTH, BARHEIGHT-3});

    unsigned twid = cached_string_width(2, input_str);
    render_string_cached(2, input_str, WIDTH/2-twid/2, HEIGHT-BARHEIGHT+8, 1.0);

    // Update CPM
    {
        // This is the index of the second that was one minute ago
        size_t sec = (SDL_GetTicks() / 1000 + 1) % 60;

        cpm -= chars_in_second[sec];
        chars_in_second[sec] = 0;
    }

    // draw wpm and stuff
    char info_str[100];

    sprintf(info_str, "WPM: %u", cpm/5);
    render_string(info_str, 5, HEIGHT-BARHEIGHT+5, 0.6);
    sprintf(info_str, "CPM: %u", cpm);
    render_string(info_str, 5, HEIGHT-BARHEIGHT+5+20, 0.6);

    sprintf(info_str, "Words : %u", words);
    render_string(info_str, WIDTH-140, HEIGHT-BARHEIGHT+5, 0.6);
    sprintf(info_str, "Chars : %u", chars);
    render_string(info_str, WIDTH-140, HEIGHT-BARHEIGHT+5+20, 0.6);

    return 1;
}

void game_render_scores(SDL_Texture** rows) {

    char buf[100];

    Uint32 survived = SDL_GetTicks() - round_start;

    unsigned hours = 0, minutes = 0, seconds = 0;
    if (survived >= 3600000) {hours = survived / 3600000; survived %= 36000000; }
    if (survived >= 60000) {minutes = survived / 60000; survived %= 60000; }
    if (survived >= 1000) {seconds = survived / 1000; }

    sprintf(buf, "Time survived : %02u:%02u:%02u", hours, minutes, seconds);
    rows[0] = string_cache(buf, (SDL_Color){200, 200, 255, 255});

    sprintf(buf, "Words : %u", words);
    rows[1] = string_cache(buf, (SDL_Color){200, 200, 255, 255});

    sprintf(buf, "Chars : %u", chars);
    rows[2] = string_cache(buf, (SDL_Color){200, 200, 255, 255});

    sprintf(buf, "Best WPM : %u", cpm_best/5);
    rows[3] = string_cache(buf, (SDL_Color){200, 200, 255, 255});

    sprintf(buf, "Best CPM : %u", cpm_best);
    rows[4] = string_cache(buf, (SDL_Color){200, 200, 255, 255});

    sprintf(buf, "Accuracy : %.1f%%", chars == 0 ? 0 : (double)chars/(chars+backspaces)*100.0);
    rows[5] = string_cache(buf, (SDL_Color){200, 200, 255, 255});
}
