//
// Created by user on 14/02/2026.
//

#include "State.h"
#include "Actor/Actors/View/View.h"
#include "utils/Utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <map>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "Actor/Actor.h"

using namespace std;

typedef function<Error(const SDL_Event&)> eventHandler;

State* State::instance = nullptr;

void State::make(const Config& config, Error& err) {
    if (instance != nullptr) {
        err = Error::New("Instance is already created.");
        return;
    }
    instance = new State(config, err);
    if (err.status == failure) {
        cerr << "Couldn't create initial state: " << err.message << endl;
        delete instance;
        instance = nullptr;
    }
}

void State::destroy() {
    delete instance;
    instance = nullptr;
}

State& State::get() {
    return *instance;
}

function<Error(const SDL_Event&)> getEventHandler(Actor::EventGroup eventGroup, function<Uint32(const SDL_Event&)> getKey) {
    return [eventGroup, getKey](const SDL_Event& event) {
        State& state = State::get();
        for (pair<string, Actor*> nameActor : state.actors) {
            Actor* actor = nameActor.second;
            if (actor == nullptr) {
                continue;
            }
            auto groupIt = actor->eventActions.find(eventGroup);
            if (groupIt == actor->eventActions.end()) {
                continue;
            }
            Uint32 key = getKey(event);
            auto actionIt = groupIt->second.find(key);
            if (actionIt == groupIt->second.end() || actionIt->second == nullptr) {
                continue;
            }
            actionIt->second(*actor, event);
        }
        return Error::Success();
    };
}

State::State(const Config& config, Error& err)
    : animations(),
      env(nullptr),
      running(true),
      actors(),
      animationCleanupEveryXFrames(1000),
      view(nullptr),
      fps(config.fps),
      controlMode(config.controlMode),
      controller(nullptr),
      controllerId(static_cast<SDL_JoystickID>(-1)),
      pollEvent(config.pollEvent),
      deferActorMutations(false),
      pendingAddActors(),
      pendingRemoveNames(),
      pendingRemoveActors() {
    env = new SDL_Environment(config, err);
    if (err.status == failure) {
        err = Error::New("Failed to set up SDL_Environment: " + err.message);
        return;
    }

    if (controlMode == GAMEPAD) {
        const int numJoysticks = SDL_NumJoysticks();
        for (int i = 0; i < numJoysticks; ++i) {
            if (!SDL_IsGameController(i)) {
                continue;
            }
            controller = SDL_GameControllerOpen(i);
            if (controller == nullptr) {
                err = Error::New(string("Failed to open game controller: ") + SDL_GetError());
                return;
            }
            controllerId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
            break;
        }
        if (controller == nullptr) {
            err = Error::New("No game controller found");
            return;
        }
    }

    view = new View(1920, 1080);
    if (!registerActor("view", view, err)) {
        return;
    }
    view->eventActions[Actor::quit][0] = [this](Actor& actor, const SDL_Event& event) {
        running = false;
    };
}

State::~State() {
    for (pair<string, Actor*> actor : actors) {
        delete actor.second;
    }
    for (const auto& nameActor : pendingAddActors) {
        delete nameActor.second;
    }
    pendingAddActors.clear();
    pendingRemoveNames.clear();
    pendingRemoveActors.clear();
    for (pair<string, Animation> nameAnimation : animations) {
        Animation& anim = nameAnimation.second;
        auto list = !anim.iterator;
        if (list == nullptr) {
            continue;
        }
        for (auto it = list->begin(); it != list->end(); ++it) {
            delete *it;
        }
        delete list;
    }
    if (controller != nullptr) {
        SDL_GameControllerClose(controller);
        controller = nullptr;
    }
    actors.clear();
    delete env;
}

void State::startEventLoop() {
    if (env == nullptr) {
        return;
    }

    Error err;

    SDL_Event event;
    map<Uint32, eventHandler> typeToHandler;
    typeToHandler[SDL_QUIT] = getEventHandler(Actor::quit, [](const SDL_Event& event) {
        static_cast<void>(event);
        return static_cast<Uint32>(0);
    });
    typeToHandler[SDL_KEYDOWN] = getEventHandler(Actor::keyDown, [](const SDL_Event& event) {
        return static_cast<Uint32>(event.key.keysym.sym);
    });
    typeToHandler[SDL_KEYUP] = getEventHandler(Actor::keyUp, [](const SDL_Event& event) {
        return static_cast<Uint32>(event.key.keysym.sym);
    });
    typeToHandler[SDL_CONTROLLERBUTTONDOWN] = getEventHandler(Actor::controllerButtonDown, [](const SDL_Event& event) {
        return static_cast<Uint32>(event.cbutton.button);
    });
    typeToHandler[SDL_CONTROLLERBUTTONUP] = getEventHandler(Actor::controllerButtonUp, [](const SDL_Event& event) {
        return static_cast<Uint32>(event.cbutton.button);
    });

    int frameCounter = 0;
    unordered_map<string, Uint32> animationLastAdvanceMs;

    const double perfFrequency = static_cast<double>(SDL_GetPerformanceFrequency());

    deferActorMutations = true;
    while (running) {
        Uint64 frameStart = SDL_GetPerformanceCounter();
        while (pollEvent(&event)) {
            auto handlerIt = typeToHandler.find(event.type);
            if (handlerIt == typeToHandler.end()) {
                continue;
            }
            eventHandler handle = handlerIt->second;
            if (handle == nullptr) {
                continue;
            }
            err = handle(event);
            if (err.status == failure) {
                cerr << "SDL_Event type error: " << err.message << endl;
            }
        }
        err = Error::Success();

        env->renderer->setDrawColor(16, 64, 32, 255);
        env->renderer->clear();

        for (pair<string, Actor*> nameActor : actors) {
            Actor* actor = nameActor.second;

            actor->position = actor->position + actor->velocity;
            actor->velocity = actor->velocity + actor->acceleration;
            if (actor->onRender != nullptr) {
                actor->onRender(*actor);
            }

            string cur = actor->currentAnimation;
            if (!cur.empty()) {
                auto animNameIt = actor->animations.find(cur);
                if (animNameIt == actor->animations.end()) {
                    continue;
                }
                const string& animationName = animNameIt->second;
                auto animIt = animations.find(animationName);
                if (animIt == animations.end()) {
                    cerr << "Missing animation: " << animationName << endl;
                    continue;
                }
                Animation& anim = animIt->second;
                auto list = !anim.iterator;
                if (list == nullptr || list->empty()) {
                    cerr << "Missing animation frames: " << animationName << endl;
                    continue;
                }
                Texture* frame = *anim.iterator;
                if (frame == nullptr) {
                    cerr << "Missing animation frame: " << animationName << endl;
                    continue;
                }
                env->renderer->copy(frame, actor->position - view->position, err);
                if (err.status == failure) {
                    cerr << "Texture copy error: " << err.message << endl;
                }
                if (anim.msPerFrame <= 0) {
                    ++anim.iterator;
                } else {
                    Uint32 nowMs = SDL_GetTicks();
                    Uint32& lastAdvance = animationLastAdvanceMs[animationName];
                    if (nowMs - lastAdvance >= static_cast<Uint32>(anim.msPerFrame)) {
                        ++anim.iterator;
                        lastAdvance = nowMs;
                    }
                }
            }
        }

        env->renderer->present();
        collisionHandler();
        applyDeferredActors();
        if (frameCounter % animationCleanupEveryXFrames == 0) {
            cleanupAnimations();
        }
        frameCounter++;

        if (fps > 0) {
            const double targetSeconds = 1.0 / static_cast<double>(fps);
            double elapsedSeconds = (SDL_GetPerformanceCounter() - frameStart) / perfFrequency;
            if (elapsedSeconds < targetSeconds) {
                double remainingSeconds = targetSeconds - elapsedSeconds;
                Uint32 delayMs = static_cast<Uint32>(remainingSeconds * 1000.0);
                if (delayMs > 0) {
                    SDL_Delay(delayMs);
                }
                while (((SDL_GetPerformanceCounter() - frameStart) / perfFrequency) < targetSeconds) {
                }
            }
        }
    }
    deferActorMutations = false;
}

void State::collisionHandler() {
    Error err;
    vector<pair<Actor*, SDL_Rect>> boxes;
    for (const auto& nameActor : actors) {
        Actor* actor = nameActor.second;
        if (actor == nullptr || actor == view) {
            continue;
        }
        int width = 0;
        int height = 0;
        if (!actor->getSize(width, height, err)) {
            err = Error::Success();
            continue;
        }
        SDL_Rect box{static_cast<int>(actor->position[0]), static_cast<int>(actor->position[1]), width, height};
        boxes.emplace_back(actor, box);
    }

    for (size_t i = 0; i < boxes.size(); ++i) {
        for (size_t j = i + 1; j < boxes.size(); ++j) {
            Actor* left = boxes[i].first;
            Actor* right = boxes[j].first;
            const SDL_Rect& a = boxes[i].second;
            const SDL_Rect& b = boxes[j].second;
            if (!rectanglesOverlap(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h)) {
                continue;
            }
            if (left->onCollision != nullptr) {
                left->onCollision(*left, *right);
            }
            if (right->onCollision != nullptr) {
                right->onCollision(*right, *left);
            }
        }
    }
}

void State::removeActor(const string& name) {
    if (deferActorMutations) {
        pendingRemoveNames.insert(name);
        for (auto it = pendingAddActors.begin(); it != pendingAddActors.end(); ++it) {
            if (it->first == name) {
                delete it->second;
                pendingAddActors.erase(it);
                break;
            }
        }
        return;
    }
    auto it = actors.find(name);
    if (it == actors.end()) {
        return;
    }
    delete it->second;
    actors.erase(it);
}

void State::removeActor(Actor* actor) {
    if (actor == nullptr) {
        return;
    }
    if (deferActorMutations) {
        for (auto it = pendingAddActors.begin(); it != pendingAddActors.end(); ++it) {
            if (it->second == actor) {
                delete it->second;
                pendingAddActors.erase(it);
                return;
            }
        }
        pendingRemoveActors.insert(actor);
        return;
    }
    for (auto it = actors.begin(); it != actors.end(); ++it) {
        if (it->second == actor) {
            if (it->second == view) {
                view = nullptr;
            }
            delete it->second;
            actors.erase(it);
            return;
        }
    }
}

Actor* State::addActor(const string& name, Error& err) {
    auto a = new Actor();
    if (!registerActor(name, a, err)) {
        delete a;
        return nullptr;
    }
    return a;
}

bool State::registerActor(const string& name, Actor* actor, Error& err) {
    if (actor == nullptr) {
        err = Error::New("Actor is null");
        return false;
    }
    if (actors.contains(name)) {
        err = Error::New(string("Actor with name ") + name + " already exists", Error::duplicate);
        return false;
    }
    for (const auto& pending : pendingAddActors) {
        if (pending.first == name) {
            err = Error::New(string("Actor with name ") + name + " already exists", Error::duplicate);
            return false;
        }
    }
    if (deferActorMutations) {
        pendingAddActors.emplace_back(name, actor);
        return true;
    }
    actors[name] = actor;
    return true;
}

Animation State::loadAnimation(const string& name, std::initializer_list<string> paths, int msPerFrame, Error& err) {
    auto anim = loadAnimationNoDuplicate(name, paths, msPerFrame, err);
    if (err.status == failure) {
        if (err.type == Error::duplicate) {
            err = Error::Success();
        }
    }
    return anim;
}

Animation State::loadAnimationNoDuplicate(const string& name, std::initializer_list<string> paths, int msPerFrame, Error& err) {
    Animation anim;
    const auto& animIt = animations.find(name);
    if (animIt != animations.end()) {
        err = Error::New(string("Animation already exists: ") + name, Error::duplicate);
        anim = animIt->second;
        return anim;
    }
    anim = env->renderer->loadAnimation(paths, msPerFrame, err);
    if (err.status == failure) {
        auto list = !anim.iterator;
        if (list != nullptr) {
            for (auto it = list->begin(); it != list->end(); ++it) {
                delete *it;
            }
            delete list;
        }
        return {};
    }
    animations[name] = anim;
    return anim;
}

Animation State::loadAnimation(const string& name, std::initializer_list<pair<string, const SDL_Rect*>> frames, int msPerFrame, Error& err) {
    auto anim = loadAnimationNoDuplicate(name, frames, msPerFrame, err);
    if (err.status == failure) {
        if (err.type == Error::duplicate) {
            err = Error::Success();
        }
    }
    return anim;
}

Animation State::loadAnimationNoDuplicate(const string& name, std::initializer_list<pair<string, const SDL_Rect*>> frames, int msPerFrame, Error& err) {
    Animation anim;
    const auto& animIt = animations.find(name);
    if (animIt != animations.end()) {
        err = Error::New(string("Animation already exists: ") + name, Error::duplicate);
        anim = animIt->second;
        return anim;
    }
    anim = env->renderer->loadAnimation(frames, msPerFrame, err);
    if (err.status == failure) {
        auto list = !anim.iterator;
        if (list != nullptr) {
            for (auto it = list->begin(); it != list->end(); ++it) {
                delete *it;
            }
            delete list;
        }
        return {};
    }
    animations[name] = anim;
    return anim;
}

void State::cleanupAnimations() {
    unordered_set<string> activeAnimations;
    for (pair<string, Actor*> nameActor : actors) {
        Actor* actor = nameActor.second;
        for (pair<string, string> animationPair : actor->animations) {
            string animationName = animationPair.second;
            activeAnimations.insert(animationName);
        }
    }

    unordered_set<string> keysToDelete;
    for (const pair<const string, Animation>& animationName : animations) {
        const string& name = animationName.first;
        const Animation& anim = animationName.second;
        if (activeAnimations.contains(name)) {
            continue;
        }

        keysToDelete.insert(name);
        auto list = !anim.iterator;
        if (list != nullptr) {
            for (Texture* frame : *list) {
                delete frame;
            }
            delete list;
        }
    }

    for (const string& key : keysToDelete) {
        animations.erase(key);
    }
}

void State::applyDeferredActors() {
    if (pendingRemoveNames.empty() && pendingRemoveActors.empty() && pendingAddActors.empty()) {
        return;
    }

    for (const string& name : pendingRemoveNames) {
        auto it = actors.find(name);
        if (it == actors.end()) {
            continue;
        }
        if (it->second == view) {
            view = nullptr;
        }
        delete it->second;
        actors.erase(it);
    }

    for (Actor* actor : pendingRemoveActors) {
        for (auto it = actors.begin(); it != actors.end(); ++it) {
            if (it->second == actor) {
                if (it->second == view) {
                    view = nullptr;
                }
                delete it->second;
                actors.erase(it);
                break;
            }
        }
    }

    pendingRemoveNames.clear();
    pendingRemoveActors.clear();

    for (const auto& nameActor : pendingAddActors) {
        const string& name = nameActor.first;
        Actor* actor = nameActor.second;
        if (actors.contains(name)) {
            delete actor;
            continue;
        }
        actors[name] = actor;
    }
    pendingAddActors.clear();
}
