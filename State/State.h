//
// Created by user on 14/02/2026.
//

#ifndef GAME_STATE_H
#define GAME_STATE_H
#include "SDL_Environment/SDL_Environment.h"
#include "Actor/Actors/View/View.h"
#include "Animation/Animation.h"
#include <initializer_list>
#include <utility>
#include <vector>
#include <SDL2/SDL.h>
#include <unordered_set>

#include "Actor/Actor.h"

using namespace std;

class View;

class State {
    public:
    //Static
    static State* instance;
    static void make(const Config& config, Error& err);
    static void destroy();
    static State& get();

    // Ctors and Dtors
    State(const State&) = delete;
    State& operator=(const State&) = delete;
    ~State();

    // Methods
    void startEventLoop();
    void collisionHandler();
    Animation loadAnimation(const string& name, std::initializer_list<string> paths, int msPerFrame, Error& err);
    Animation loadAnimation(const string& name, std::initializer_list<pair<string, const SDL_Rect*>> frames, int msPerFrame, Error& err);
    Animation loadAnimationNoDuplicate(const string& name, std::initializer_list<string> paths, int msPerFrame, Error& err);
    Animation loadAnimationNoDuplicate(const string& name, std::initializer_list<pair<string, const SDL_Rect*>> frames, int msPerFrame, Error& err);
    Animation loadAnimationFrames(const string& name, const vector<string>& paths, int msPerFrame, Error& err);
    void restartAnimation(const string& name);
    Actor* addActor(const string& name, Error& err);
    bool registerActor(const string& name, Actor* actor, Error& err);

    void removeActor(const string& name);
    void removeActor(Actor* actor);

    void cleanupAnimations();
    void applyDeferredActors();

    // Vars
    map<string, Animation> animations;
    SDL_Environment* env;
    bool running;
    // While true, actors marked pausable stop moving and animating.
    bool paused;
    map<string, Actor*> actors;
    int animationCleanupEveryXFrames;
    View* view;
    SDL_Color background;
    int fps;
    ControlMode controlMode;
    SDL_GameController* controller;
    SDL_JoystickID controllerId;

    function<int(SDL_Event *)> pollEvent;
    // Drawn on top of everything each frame (menus, HUD).
    function<void()> onOverlay;

    bool deferActorMutations;
    vector<pair<string, Actor*>> pendingAddActors;
    unordered_set<string> pendingRemoveNames;
    unordered_set<Actor*> pendingRemoveActors;

    protected:
    State(const Config& config, Error& err);
};


#endif //GAME_STATE_H
