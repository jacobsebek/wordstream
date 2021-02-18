#pragma once

#include <stddef.h>
#include <stdio.h>

#define	WORDLEN 12

char** dict_load(FILE* f, size_t* size);
void dict_destroy(char** dict, size_t size);
