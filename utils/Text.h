#ifndef GAME_TEXT_H
#define GAME_TEXT_H
#include <SDL2/SDL.h>
#include <string>
using namespace std;

// Tiny built-in 5x7 pixel font (upper case letters, digits and a few symbols),
// so the game needs no font files. Lower case is drawn as upper case.
// scale is the size of one font pixel on screen.
int textWidth(const string& text, int scale);
int textHeight(int scale);
void drawText(SDL_Renderer* renderer, const string& text, int x, int y, int scale, SDL_Color color);
// Draws the text horizontally centred on centerX.
void drawTextCentered(SDL_Renderer* renderer, const string& text, int centerX, int y, int scale, SDL_Color color);

#endif //GAME_TEXT_H
