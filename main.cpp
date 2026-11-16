#include <iostream>
#include <SDL2/SDL.h>
#include "Error.h"
#include "Actor/Actors/Level/Level.h"
#include "Actor/Actors/Player/Player.h"
#include "SDL_Environment/SDL_Environment.h"
#include "State/State.h"
#include "Vector/Vector.h"

using namespace std;

int main()
{
    Error err;
    auto config = getDefaultConfig();

    State::make(config, err);
    if (err.status == failure) {
        cerr << "SDL_Environment create error: " << err.message << endl;
        return 1;
    }
    State& state = State::get();

    Level* level = new Level();
    state.registerActor("level", level, err);
    if (err.status == failure) {
        delete level;
        cerr << "Failed to create level: " << err.message << endl;
        State::destroy();
        return 1;
    }

    Player* player = new Player("player", Vector(200, 300), level, err);
    if (err.status == failure) {
        delete player;
        cerr << "Failed to create player: " << err.message << endl;
        State::destroy();
        return 1;
    }

    player->controlMode = state.controlMode;
    player->controllerId = state.controllerId;

    state.startEventLoop();
    State::destroy();
    return 0;
}
