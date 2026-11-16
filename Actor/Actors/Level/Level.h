#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H
#include "Actor/Actor.h"
#include <vector>
using namespace std;


// Placeholder level: the ground and the platforms are plain white lines.
// Every line is solid from above only (actors can jump up through it and land on top).
class Level : public Actor {
    public:
    Level();
    ~Level();

    // Rebuilds the platforms (and colours) for level number 1-10.
    void load(int number);
    int number;
    // Total width of the level in pixels; its left end is x=0, its right end is x=width.
    int width;
    SDL_Color backgroundColor;
    SDL_Color lineColor;

    // Thickness of the drawn lines in pixels.
    int thickness;
    // Each rect is one line; rect.y is the top surface actors stand on.
    vector<SDL_Rect> lines;
};


#endif //GAME_LEVEL_H
