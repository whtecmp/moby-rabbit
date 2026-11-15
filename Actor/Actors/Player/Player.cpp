//
// Created by user on 19/02/2026.
//

#include "Player.h"
#include "State/State.h"
#include <SDL2/SDL_gamecontroller.h>
#include <iostream>
using namespace std;

// Path is relative to the working directory. Assets are not stored in git.
static const string playerSpritePath = "./assets/player.png";

Actor::eventAction Player::genOnMoveStart(const Vector& direction) {
    return [this, direction](Actor& actor, const SDL_Event& event) {
        static_cast<void>(actor);
        static_cast<void>(event);
        if (direction[0] != 0.0) {
            velocity[0] = direction[0] * speed;
        }
        if (direction[1] != 0.0) {
            velocity[1] = direction[1] * speed;
        }
    };
}

Actor::eventAction Player::genOnMoveStop(const Vector& direction) {
    return [this, direction](Actor& actor, const SDL_Event& event) {
        static_cast<void>(actor);
        static_cast<void>(event);
        // Only stop if we are still moving in the released direction.
        if (direction[0] != 0.0 && velocity[0] * direction[0] > 0.0) {
            velocity[0] = 0.0;
        }
        if (direction[1] != 0.0 && velocity[1] * direction[1] > 0.0) {
            velocity[1] = 0.0;
        }
    };
}

Player::Player(const string& name, const Vector& startPosition, Error& err)
    : Actor(), controlMode(KEYBOARD), controllerId(static_cast<SDL_JoystickID>(-1)), speed(5) {
    State& state = State::get();
    if (state.actors.contains(name)) {
        err = Error::New(string("Player with name ") + name + " already exists", Error::duplicate);
        return;
    }

    // A missing sprite is not fatal: the player still moves, it just isn't drawn.
    Error animErr;
    state.loadAnimation("player_idle", {playerSpritePath}, 0, animErr);
    if (animErr.status == failure) {
        cerr << "Player sprite not loaded (" << playerSpritePath << "): " << animErr.message << endl;
    } else {
        animations["idle"] = "player_idle";
        currentAnimation = "idle";
    }

    position = startPosition;
    acceleration = Vector(0, 0);

    auto isControlledByKeyboardDecorator = [this](const Actor::eventAction& action) {
        return [this, action](Actor& actor, const SDL_Event& event) {
            if (controlMode != KEYBOARD) {
                return;
            }
            action(actor, event);
        };
    };
    auto isControlledByGamepadDecorator = [this](const Actor::eventAction& action) {
        return [this, action](Actor& actor, const SDL_Event& event) {
            if (controlMode != GAMEPAD) {
                return;
            }
            if (controllerId != event.cbutton.which) {
                return;
            }
            action(actor, event);
        };
    };

    const Vector left(-1, 0);
    const Vector right(1, 0);
    const Vector up(0, -1);
    const Vector down(0, 1);

    // Keyboard event handlers
    eventActions[Actor::keyDown][SDLK_LEFT] = isControlledByKeyboardDecorator(genOnMoveStart(left));
    eventActions[Actor::keyDown][SDLK_RIGHT] = isControlledByKeyboardDecorator(genOnMoveStart(right));
    eventActions[Actor::keyDown][SDLK_UP] = isControlledByKeyboardDecorator(genOnMoveStart(up));
    eventActions[Actor::keyDown][SDLK_DOWN] = isControlledByKeyboardDecorator(genOnMoveStart(down));
    eventActions[Actor::keyUp][SDLK_LEFT] = isControlledByKeyboardDecorator(genOnMoveStop(left));
    eventActions[Actor::keyUp][SDLK_RIGHT] = isControlledByKeyboardDecorator(genOnMoveStop(right));
    eventActions[Actor::keyUp][SDLK_UP] = isControlledByKeyboardDecorator(genOnMoveStop(up));
    eventActions[Actor::keyUp][SDLK_DOWN] = isControlledByKeyboardDecorator(genOnMoveStop(down));

    // Gamepad event handlers
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_LEFT] = isControlledByGamepadDecorator(genOnMoveStart(left));
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = isControlledByGamepadDecorator(genOnMoveStart(right));
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_UP] = isControlledByGamepadDecorator(genOnMoveStart(up));
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_DOWN] = isControlledByGamepadDecorator(genOnMoveStart(down));
    eventActions[Actor::controllerButtonUp][SDL_CONTROLLER_BUTTON_DPAD_LEFT] = isControlledByGamepadDecorator(genOnMoveStop(left));
    eventActions[Actor::controllerButtonUp][SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = isControlledByGamepadDecorator(genOnMoveStop(right));
    eventActions[Actor::controllerButtonUp][SDL_CONTROLLER_BUTTON_DPAD_UP] = isControlledByGamepadDecorator(genOnMoveStop(up));
    eventActions[Actor::controllerButtonUp][SDL_CONTROLLER_BUTTON_DPAD_DOWN] = isControlledByGamepadDecorator(genOnMoveStop(down));

    if (!state.registerActor(name, this, err)) {
        return;
    }
}

Player::~Player() = default;
