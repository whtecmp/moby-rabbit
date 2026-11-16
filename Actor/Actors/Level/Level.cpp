#include "Level.h"
#include "State/State.h"
#include "Save/Save.h"
#include <algorithm>
#include <cmath>
using namespace std;

Level::Level() : Actor(), number(1), width(4800),
                 backgroundColor{0, 70, 30, 255}, lineColor{255, 255, 255, 255},
                 goalX(0), goalY(0), goalRadius(50), thickness(4), lines() {
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
        // Goal: a solid yellow circle drawn as horizontal slices.
        state.env->renderer->setDrawColor(255, 220, 0, 255);
        const int cx = goalX - static_cast<int>(state.view->position[0]);
        const int cy = goalY - static_cast<int>(state.view->position[1]);
        for (int dy = -goalRadius; dy <= goalRadius; ++dy) {
            const int half = static_cast<int>(sqrt(static_cast<double>(goalRadius * goalRadius - dy * dy)));
            SDL_RenderDrawLine(renderer, cx - half, cy + dy, cx + half, cy + dy);
        }
    };
    onPausedRender = onRender;
}

Level::~Level() = default;

bool Level::goalTouches(const SDL_Rect& box) const {
    // Closest point of the box to the circle centre.
    const int nearestX = max(box.x, min(goalX, box.x + box.w));
    const int nearestY = max(box.y, min(goalY, box.y + box.h));
    const int dx = goalX - nearestX;
    const int dy = goalY - nearestY;
    return dx * dx + dy * dy <= goalRadius * goalRadius;
}

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
    goalX = width - 150;
    goalY = 900 - goalRadius - 10;
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

