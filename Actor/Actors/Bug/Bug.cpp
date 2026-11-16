#include "Bug.h"
#include "State/State.h"
using namespace std;

static bool overlap(const SDL_Rect& a, const SDL_Rect& b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

Bug::Bug(const string& name, double minX, double maxX, double feetY, Player* player, Error& err)
    : Actor(), player(player), minX(minX), maxX(maxX), speed(3), movingRight(true), alive(true),
      width(64), height(48) {
    position = Vector(minX, feetY - height);
    pausable = true;
    onRender = [this](Actor& actor) {
        static_cast<void>(actor);
        update();
        draw();
    };
    onPausedRender = [this](Actor& actor) {
        static_cast<void>(actor);
        draw();
    };
    State::get().registerActor(name, this, err);
}

Bug::~Bug() = default;

void Bug::update() {
    if (!alive) {
        return;
    }
    // Fixed back-and-forth pattern between minX and maxX.
    if (movingRight && position[0] >= maxX) {
        movingRight = false;
    } else if (!movingRight && position[0] <= minX) {
        movingRight = true;
    }
    velocity[0] = movingRight ? speed : -speed;

    if (player == nullptr) {
        return;
    }
    const SDL_Rect box{static_cast<int>(position[0]), static_cast<int>(position[1]), width, height};
    SDL_Rect attack;
    if (player->attackBox(attack) && overlap(attack, box)) {
        // TODO: play a dying animation once it exists.
        alive = false;
        State::get().removeActor(this);
        return;
    }
    if (overlap(player->bodyBox(), box)) {
        player->takeHit();
    }
}

void Bug::draw() const {
    if (!alive) {
        return;
    }
    State& state = State::get();
    SDL_Renderer* renderer = state.env->renderer->renderer;
    const int x = static_cast<int>(position[0] - state.view->position[0]);
    const int y = static_cast<int>(position[1] - state.view->position[1]);
    SDL_Rect body{x, y, width, height};
    state.env->renderer->setDrawColor(40, 20, 60, 255);
    SDL_RenderFillRect(renderer, &body);
    // Two white eyes on the side it is walking towards.
    const int eyeX = movingRight ? x + width - 24 : x + 8;
    SDL_Rect eyeA{eyeX, y + 10, 8, 8};
    SDL_Rect eyeB{eyeX + 8, y + 10, 8, 8};
    state.env->renderer->setDrawColor(255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &eyeA);
    SDL_RenderFillRect(renderer, &eyeB);
}
