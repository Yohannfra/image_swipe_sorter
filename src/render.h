#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>

/* Render text using bitmap font */
void render_text(SDL_Renderer *renderer, const char *text, int x, int y, int scale);

/* Render arrow indicator. direction: -1 = left, 1 = right */
void render_arrow(SDL_Renderer *renderer, int x, int y, int size, int direction);

#endif /* RENDER_H */
