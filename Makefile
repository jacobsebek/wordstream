EXEC=./wordstream.exe
VPATH=src

SDL_CONFIG?=/usr/local/bin/sdl2-config

CFLAGS=-Wall -Wextra -std=c99 -pedantic -Iinclude `${SDL_CONFIG} --cflags`
LDLIBS=-lSDL2_ttf -lSDL2_mixer `$(SDL_CONFIG) --libs` 

OBJECTS=$(patsubst %.c, %.o, $(notdir $(wildcard $(VPATH)/*.c)))

$(EXEC) : $(OBJECTS)
	${CC} -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

%.o : include/*.h
