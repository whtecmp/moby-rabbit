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

    // Thickness of the drawn lines in pixels.
    int thickness;
    // Each rect is one line; rect.y is the top surface actors stand on.
    vector<SDL_Rect> lines;
};


#endif //GAME_LEVEL_H
