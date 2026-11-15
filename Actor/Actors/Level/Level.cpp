#include "Level.h"
#include "State/State.h"
using namespace std;

Level::Level() : Actor(), thickness(4), lines() {
    // Ground first, then platforms to run along and jump between.
    lines.push_back({0, 900, 1920, thickness});
    lines.push_back({200, 720, 400, thickness});
    lines.push_back({800, 600, 350, thickness});
    lines.push_back({1350, 720, 400, thickness});
    lines.push_back({500, 450, 350, thickness});
    lines.push_back({1100, 340, 400, thickness});

    onRender = [this](Actor& actor) {
        static_cast<void>(actor);
        State& state = State::get();
        SDL_Renderer* renderer = state.env->renderer->renderer;
        state.env->renderer->setDrawColor(255, 255, 255, 255);
        for (const SDL_Rect& line : lines) {
            SDL_Rect r = line;
            r.x -= static_cast<int>(state.view->position[0]);
            r.y -= static_cast<int>(state.view->position[1]);
            SDL_RenderFillRect(renderer, &r);
        }
    };
}

Level::~Level() = default;
