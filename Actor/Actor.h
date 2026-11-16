//
// Created by user on 14/02/2026.
//

#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H
#include "Texture/Texture.h"
#include "Vector/Vector.h"
#include <SDL2/SDL.h>
#include <functional>
#include <map>
#include <memory>
#include <string>

using namespace std;

class State;


class Actor {
public:
    using eventAction = function<void(Actor&, const SDL_Event&)>;

    enum EventGroup {
        keyDown,
        keyUp,
        quit,
        windowEvent,
        mouseMotion,
        mouseButtonDown,
        mouseButtonUp,
        mouseWheel,
        textInput,
        textEditing,
        joystick,
        controllerButtonDown,
        controllerButtonUp,
        touch,
        gesture,
        drop,
        userEvent,
        audioDevice
    };

    Actor();
    virtual ~Actor();

    bool getSize(int& width, int& height, Error& err) const;

    double mass;
    Vector position;
    Vector velocity;
    Vector acceleration;

    // When false the actor is skipped completely (no movement, logic or drawing).
    bool active;
    // When false the actor is not drawn (its logic still runs).
    bool visible;
    // Pausable actors stand still while State::paused is set.
    bool pausable;
    // Used instead of onRender while the game is paused (e.g. to keep drawing a level).
    function<void(Actor&)> onPausedRender;

    string currentAnimation;
    // Drawing tweaks: mirror the sprite and shift it relative to position.
    bool flipHorizontal;
    Vector drawOffset;
    map<string, string> animations;
    map<EventGroup, map<Uint32, eventAction>> eventActions;
    function<void(Actor&)> onRender;
    // Called once per frame for every other actor whose bounding box overlaps this one.
    function<void(Actor&, Actor&)> onCollision;
};


#endif //GAME_ACTOR_H
