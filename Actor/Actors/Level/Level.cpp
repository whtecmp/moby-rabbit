#include "Level.h"
#include "State/State.h"
#include "Save/Save.h"
using namespace std;

Level::Level() : Actor(), number(1), width(4800),
                 backgroundColor{0, 70, 30, 255}, lineColor{255, 255, 255, 255}, thickness(4), lines() {
    load(1);

    onRender = [this](Actor& actor) {
        static_cast<void>(actor);
        State& state = State::get();
        SDL_Renderer* renderer = state.env->renderer->renderer;
        state.env->renderer->setDrawColor(lineColor.r, lineColor.g, lineColor.b, 255);
        for (const SDL_Rect& line : lines) {
            SDL_Rect r = line;
            r.x -= static_cast<int>(state.view->position[0]);
            r.y -= static_cast<int>(state.view->position[1]);
            SDL_RenderFillRect(renderer, &r);
        }
    };
    onPausedRender = onRender;
}

Level::~Level() = default;

void Level::load(int levelNumber) {
    number = levelNumber;
    static const SDL_Color palettes[maxLevel][2] = {
        {{0, 70, 30, 255}, {255, 255, 255, 255}},
        {{20, 40, 90, 255}, {200, 230, 255, 255}},
        {{90, 30, 30, 255}, {255, 220, 150, 255}},
        {{60, 20, 90, 255}, {255, 180, 255, 255}},
        {{110, 80, 20, 255}, {255, 255, 200, 255}},
        {{10, 80, 90, 255}, {180, 255, 240, 255}},
        {{40, 40, 40, 255}, {255, 120, 60, 255}},
        {{90, 90, 20, 255}, {30, 20, 0, 255}},
        {{120, 40, 80, 255}, {255, 255, 255, 255}},
        {{15, 15, 45, 255}, {160, 255, 160, 255}},
    };
    const int paletteIndex = (levelNumber - 1 + maxLevel) % maxLevel;
    backgroundColor = palettes[paletteIndex][0];
    lineColor = palettes[paletteIndex][1];
    lines.clear();
    bugs.clear();
    // Ground first, then platforms to run along and jump between.
    lines.push_back({0, 900, width, thickness});
    // One block of platforms is repeated along the level.
    const SDL_Rect block[] = {{200, 720, 400, thickness}, {800, 600, 350, thickness},
                              {1350, 720, 400, thickness}, {500, 450, 350, thickness},
                              {1100, 340, 400, thickness}};
    for (int offset = 0; offset + 1750 < width - 300; offset += 1600) {
        for (const SDL_Rect& r : block) {
            lines.push_back({r.x + offset, r.y, r.w, r.h});
        }
        bugs.push_back({offset + 1000.0, offset + 1500.0, 900});
        bugs.push_back({offset + 820.0, offset + 1090.0, 600});
    }
}

