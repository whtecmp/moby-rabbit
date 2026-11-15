//
// Created by user on 19/02/2026.
//

#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H
#include "Actor/Actor.h"
#include "State/State.h"
#include "Config/Config.h"
using namespace std;


// The single player-controlled actor. Moves with the arrow keys (KEYBOARD)
// or the D-pad (GAMEPAD), and doubles as the reference example of how an
// actor loads animations, binds event actions and registers with State.
class Player : public Actor {
    public:
    Player(const string& name, const Vector& startPosition, Error& err);
    ~Player();

    Actor::eventAction genOnMoveStart(const Vector& direction);
    Actor::eventAction genOnMoveStop(const Vector& direction);

    ControlMode controlMode;
    SDL_JoystickID controllerId;
    double speed;
};


#endif //GAME_PLAYER_H
