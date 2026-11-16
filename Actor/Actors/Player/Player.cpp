//
// Created by user on 19/02/2026.
//

#include "Player.h"
#include "State/State.h"
#include <SDL2/SDL_gamecontroller.h>
#include <cstdio>
#include <iostream>
using namespace std;

// Paths are relative to the working directory. Assets are not stored in git.
static const string assetDir = "./assets/animations/pirate_mouse/";

// Sprite geometry (see the asset READMEs): idle/run frames are 128x128 with the
// feet on row 126; jump frames are 128x192 bottom-aligned; attack frames are
// 192x128 and left-aligned, so the sword reaches to the right of the body.
static const int bodySize = 128;
static const int jumpExtraHeight = 64;
static const int attackExtraWidth = 64;
static const int bodyLeft = 32;   // horizontal extent of the body used for standing on lines
static const int bodyRight = 96;
static const int feetOffset = 127;
static const int attackMs = 500;
static const int screenWidth = 1920;
// The player may drift this far from the screen centre before the view follows.
static const int centerSlack = 160;

static vector<string> framePaths(const string& dir, int from, int to) {
    vector<string> paths;
    for (int i = from; i <= to; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "frame_%02d.png", i);
        paths.push_back(assetDir + dir + "/" + name);
    }
    return paths;
}

void Player::setAnimation(const string& name) {
    if (currentAnimation != name) {
        currentAnimation = name;
        State::get().restartAnimation(animations[name]);
    }
}

void Player::startJump() {
    if (!onGround) {
        return;
    }
    velocity[1] = -jumpSpeed;
    onGround = false;
}

void Player::startAttack() {
    if (attacking) {
        return;
    }
    attacking = true;
    attackStartMs = SDL_GetTicks();
    currentAnimation = "";
    setAnimation("attack");
}

// Keeps the player near the screen centre; the view only moves once he leaves the
// slack zone, and never shows anything outside the level.
void Player::updateCamera(int worldWidth) {
    State& state = State::get();
    if (state.view == nullptr) {
        return;
    }
    const double centerX = position[0] + bodySize / 2.0;
    double viewX = state.view->position[0];
    const double mid = screenWidth / 2.0;
    if (centerX - viewX > mid + centerSlack) {
        viewX = centerX - mid - centerSlack;
    } else if (centerX - viewX < mid - centerSlack) {
        viewX = centerX - mid + centerSlack;
    }
    const double maxViewX = worldWidth > screenWidth ? worldWidth - screenWidth : 0;
    if (viewX > maxViewX) viewX = maxViewX;
    if (viewX < 0) viewX = 0;
    state.view->position[0] = viewX;
}

// Runs every frame after State has applied velocity to position.
void Player::update() {
    if (attacking && SDL_GetTicks() - attackStartMs >= static_cast<Uint32>(attackMs)) {
        attacking = false;
    }

    // Horizontal movement: locked while attacking on the ground.
    double dir = 0;
    if (leftHeld) dir -= 1;
    if (rightHeld) dir += 1;
    if (attacking && onGround) {
        dir = 0;
    }
    velocity[0] = dir * speed;
    if (!attacking && dir != 0) {
        facingRight = dir > 0;
    }
    if (position[0] < 0) position[0] = 0;
    const int worldWidth = level != nullptr ? level->width : screenWidth;
    if (position[0] > worldWidth - bodySize) position[0] = worldWidth - bodySize;

    // Land on a line only when falling through its top surface.
    const double feetY = position[1] + feetOffset;
    onGround = false;
    if (velocity[1] >= 0 && level != nullptr) {
        for (const SDL_Rect& line : level->lines) {
            const double left = position[0] + bodyLeft;
            const double right = position[0] + bodyRight;
            if (right < line.x || left > line.x + line.w) {
                continue;
            }
            if (prevFeetY <= line.y && feetY >= line.y) {
                position[1] = line.y - feetOffset;
                velocity[1] = 0;
                onGround = true;
                break;
            }
        }
    }
    prevFeetY = position[1] + feetOffset;

    updateCamera(worldWidth);

    // Pick the animation and where to draw it.
    drawOffset = Vector(0, 0);
    flipHorizontal = !facingRight;
    if (attacking) {
        setAnimation("attack");
        if (!facingRight) {
            drawOffset = Vector(-attackExtraWidth, 0);
        }
    } else if (!onGround) {
        if (velocity[1] < -3) {
            setAnimation("jump_rise");
        } else if (velocity[1] <= 3) {
            setAnimation("jump_apex");
        } else {
            setAnimation("jump_fall");
        }
        drawOffset = Vector(0, -jumpExtraHeight);
    } else if (dir != 0) {
        setAnimation("run");
    } else {
        setAnimation("idle");
    }
}

Player::Player(const string& name, const Vector& startPosition, Level* level, Error& err)
    : Actor(), controlMode(KEYBOARD), controllerId(static_cast<SDL_JoystickID>(-1)), level(level),
      speed(8), jumpSpeed(21), gravity(0.9), leftHeld(false), rightHeld(false), onGround(false),
      facingRight(true), attacking(false), attackStartMs(0), prevFeetY(0) {
    State& state = State::get();
    if (state.actors.contains(name)) {
        err = Error::New(string("Player with name ") + name + " already exists", Error::duplicate);
        return;
    }

    // Missing sprites are not fatal: the player still plays, it just isn't drawn.
    struct Spec { const char* key; const char* dir; int from; int to; int ms; };
    const Spec specs[] = {
        {"idle", "idle_breathe_right", 1, 7, 250},
        {"run", "run_right", 1, 8, 70},
        {"jump_rise", "jump_up", 5, 7, 80},
        {"jump_apex", "jump_up", 8, 8, 0},
        {"jump_fall", "jump_up", 9, 12, 60},
        {"attack", "attack_right", 1, 18, attackMs / 18},
    };
    for (const Spec& spec : specs) {
        Error animErr;
        const string animName = string("player_") + spec.key;
        state.loadAnimationFrames(animName, framePaths(spec.dir, spec.from, spec.to), spec.ms, animErr);
        if (animErr.status == failure) {
            cerr << "Player animation not loaded (" << spec.dir << "): " << animErr.message << endl;
            continue;
        }
        animations[spec.key] = animName;
    }
    currentAnimation = animations.contains("idle") ? "idle" : "";

    position = startPosition;
    acceleration = Vector(0, gravity);
    prevFeetY = position[1] + feetOffset;

    // Actions are tied to the controller that is plugged in; keyboard always works.
    auto gamepadOnly = [this](const Actor::eventAction& action) {
        return [this, action](Actor& actor, const SDL_Event& event) {
            if (controllerId == static_cast<SDL_JoystickID>(-1) || controllerId != event.cbutton.which) {
                return;
            }
            action(actor, event);
        };
    };
    auto setHeld = [this](bool* flag, bool value) -> Actor::eventAction {
        return [flag, value](Actor& actor, const SDL_Event& event) {
            static_cast<void>(actor);
            static_cast<void>(event);
            *flag = value;
        };
    };
    Actor::eventAction jump = [this](Actor& actor, const SDL_Event& event) {
        static_cast<void>(actor);
        static_cast<void>(event);
        startJump();
    };
    Actor::eventAction attack = [this](Actor& actor, const SDL_Event& event) {
        static_cast<void>(actor);
        static_cast<void>(event);
        startAttack();
    };

    // Keyboard
    for (Uint32 key : {static_cast<Uint32>(SDLK_LEFT), static_cast<Uint32>(SDLK_a)}) {
        eventActions[Actor::keyDown][key] = setHeld(&leftHeld, true);
        eventActions[Actor::keyUp][key] = setHeld(&leftHeld, false);
    }
    for (Uint32 key : {static_cast<Uint32>(SDLK_RIGHT), static_cast<Uint32>(SDLK_d)}) {
        eventActions[Actor::keyDown][key] = setHeld(&rightHeld, true);
        eventActions[Actor::keyUp][key] = setHeld(&rightHeld, false);
    }
    for (Uint32 key : {static_cast<Uint32>(SDLK_SPACE), static_cast<Uint32>(SDLK_UP), static_cast<Uint32>(SDLK_w)}) {
        eventActions[Actor::keyDown][key] = jump;
    }
    for (Uint32 key : {static_cast<Uint32>(SDLK_x), static_cast<Uint32>(SDLK_z)}) {
        eventActions[Actor::keyDown][key] = attack;
    }

    // Gamepad
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_LEFT] = gamepadOnly(setHeld(&leftHeld, true));
    eventActions[Actor::controllerButtonUp][SDL_CONTROLLER_BUTTON_DPAD_LEFT] = gamepadOnly(setHeld(&leftHeld, false));
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = gamepadOnly(setHeld(&rightHeld, true));
    eventActions[Actor::controllerButtonUp][SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = gamepadOnly(setHeld(&rightHeld, false));
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_A] = gamepadOnly(jump);
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_UP] = gamepadOnly(jump);
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_X] = gamepadOnly(attack);
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_B] = gamepadOnly(attack);

    onRender = [this](Actor& actor) {
        static_cast<void>(actor);
        update();
    };

    if (!state.registerActor(name, this, err)) {
        return;
    }
}

Player::~Player() = default;
