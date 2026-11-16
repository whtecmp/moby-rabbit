#include "Level.h"
#include "State/State.h"
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
}

Level::~Level() = default;

void Level::load(int levelNumber) {
    number = levelNumber;
    lines.clear();
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
    }
}

