
#include <stdlib.h> // random, dynamic allocation
#include <stdio.h> //file
#include <ctype.h> //isalpha, tolower
#include <string.h> //strcpy

#include "dict.h"

char** dict_load(FILE* f, size_t* size) {

    *size = 0; 

    if (f == NULL)
        return NULL;

    *size = 1;
    char** dict = malloc((*size) * sizeof(char*));
    if (!dict) return NULL;

    char str[WORDLEN];    
    size_t i = 0;
    //TODO: replace fscanf with own whitespace-separating function
    while (fscanf(f, "%11s", str) != EOF) {

        // This prevents splitting a word into two parts if it's too long
        for (int c; (c = fgetc(f)) != EOF && !isspace(c); )
        ;

        // Scrap any words that contain other characters than a-z
        _Bool scrap = 0;
        for (char* c = str; *c; c++)
            if (!isalpha(*c)) { 
                scrap = 1; 
                break; 
            } else *c = tolower(*c);
        
        if (scrap)
            continue;

        while (i >= *size) {

            size_t old_dict_size = *size;
            char** old_dict = dict;

            dict = realloc(dict, (*size*=2) * sizeof(char*));
            if (!dict) {
                dict_destroy(old_dict, old_dict_size);
                return NULL;
            }
        }

        dict[i] = strdup(str);

        i++;
    }

    // shrink the dictionary to exactly it's size
    *size = i;
    {
        char** old_dict = dict;
        dict = realloc(dict, *size * sizeof(char*));
        if (!dict) {
            dict_destroy(old_dict, *size); 
            return NULL;
        }
    }

    return dict;
}

void dict_destroy(char** dict, size_t size) {
    for (size_t w = 0; w < size; w++)
        free(dict[w]);
    free(dict);
}

