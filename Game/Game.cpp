#include "Game.h"
#include "Actor/Actors/Bug/Bug.h"
#include "Save/Save.h"
#include "State/State.h"
#include "utils/Text.h"
#include <SDL2/SDL_gamecontroller.h>
#include <iostream>
using namespace std;

static const SDL_Color white{255, 255, 255, 255};
static const SDL_Color black{0, 0, 0, 255};
static const SDL_Color menuBackground{70, 110, 160, 255};

Game::Game(Level* level, Player* player, Error& err)
    : Actor(), level(level), player(player), mode(mainMenu), selected(0), unlocked(1),
      currentLevel(1), bugCounter(0), bugNames(), savePath("save.txt") {
    State& state = State::get();
    unlocked = loadUnlockedLevel(savePath);

    auto bind = [this](Actor::EventGroup group, Uint32 key, void (Game::*action)()) {
        eventActions[group][key] = [this, action](Actor& actor, const SDL_Event& event) {
            static_cast<void>(actor);
            static_cast<void>(event);
            (this->*action)();
        };
    };
    auto up = [this](Actor&, const SDL_Event&) { moveSelection(-1); };
    auto down = [this](Actor&, const SDL_Event&) { moveSelection(1); };
    for (Uint32 key : {static_cast<Uint32>(SDLK_UP), static_cast<Uint32>(SDLK_w)}) {
        eventActions[Actor::keyDown][key] = up;
    }
    for (Uint32 key : {static_cast<Uint32>(SDLK_DOWN), static_cast<Uint32>(SDLK_s)}) {
        eventActions[Actor::keyDown][key] = down;
    }
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_UP] = up;
    eventActions[Actor::controllerButtonDown][SDL_CONTROLLER_BUTTON_DPAD_DOWN] = down;
    bind(Actor::keyDown, SDLK_RETURN, &Game::activate);
    bind(Actor::keyDown, SDLK_KP_ENTER, &Game::activate);
    bind(Actor::controllerButtonDown, SDL_CONTROLLER_BUTTON_A, &Game::activate);
    bind(Actor::keyDown, SDLK_ESCAPE, &Game::back);
    bind(Actor::controllerButtonDown, SDL_CONTROLLER_BUTTON_B, &Game::back);

    state.onOverlay = [this]() { drawOverlay(); };
    showMainMenu();
    state.registerActor("game", this, err);
}

Game::~Game() {
    State::get().onOverlay = nullptr;
}

void Game::startLevel(int number) {
    State& state = State::get();
    currentLevel = number;
    level->load(number);
    state.background = level->backgroundColor;

    for (const string& name : bugNames) {
        state.removeActor(name);
    }
    bugNames.clear();
    player->reset();
    Error err;
    for (const Level::BugSpawn& spawn : level->bugs) {
        const string name = "bug_" + to_string(bugCounter++);
        new Bug(name, spawn.minX, spawn.maxX, spawn.feetY, player, err);
        if (err.status == failure) {
            cerr << "Failed to create bug: " << err.message << endl;
            err = Error::Success();
            continue;
        }
        bugNames.push_back(name);
    }

    mode = playing;
    state.paused = false;
    level->active = true;
    player->active = true;
}

void Game::showMainMenu() {
    State& state = State::get();
    // Gameplay actors sleep behind the menu.
    for (const string& name : bugNames) {
        state.removeActor(name);
    }
    bugNames.clear();
    level->active = false;
    player->active = false;
    state.background = menuBackground;
    mode = mainMenu;
    selected = 0;
}

void Game::showLevelSelect() {
    mode = levelSelect;
    selected = 0;
    // Start on the latest unlocked level.
    selected = unlocked - 1;
}

vector<Game::MenuItem> Game::menuItems() const {
    vector<MenuItem> items;
    if (mode == mainMenu) {
        items.push_back({"Continue", true});
        items.push_back({"Choose a Level", true});
        items.push_back({"Exit", true});
    } else if (mode == levelSelect) {
        for (int i = 1; i <= maxLevel; ++i) {
            items.push_back({"Level " + to_string(i), i <= unlocked});
        }
        items.push_back({"Back", true});
    }
    return items;
}

void Game::moveSelection(int step) {
    if (mode == playing) {
        return;
    }
    const vector<MenuItem> items = menuItems();
    const int count = static_cast<int>(items.size());
    for (int tries = 0; tries < count; ++tries) {
        selected = (selected + step + count) % count;
        if (items[selected].selectable) {
            return;
        }
    }
}

void Game::activate() {
    if (mode == playing) {
        return;
    }
    const vector<MenuItem> items = menuItems();
    if (selected < 0 || selected >= static_cast<int>(items.size()) || !items[selected].selectable) {
        return;
    }
    if (mode == mainMenu) {
        if (selected == 0) {
            startLevel(unlocked);
        } else if (selected == 1) {
            showLevelSelect();
        } else {
            State::get().running = false;
        }
    } else if (mode == levelSelect) {
        if (selected < maxLevel) {
            startLevel(selected + 1);
        } else {
            showMainMenu();
        }
    }
}

void Game::back() {
    if (mode == levelSelect) {
        showMainMenu();
    }
}

void Game::drawOverlay() const {
    if (mode == playing) {
        return;
    }
    State& state = State::get();
    SDL_Renderer* renderer = state.env->renderer->renderer;
    const int centerX = state.view->width / 2;
    drawTextCentered(renderer, "Moby Rabbit", centerX, 120, 12, black);
    const vector<MenuItem> items = menuItems();
    const int scale = 5;
    const int rowHeight = textHeight(scale) + 22;
    const int top = mode == mainMenu ? 380 : 260;
    for (size_t i = 0; i < items.size(); ++i) {
        const int y = top + static_cast<int>(i) * rowHeight;
        const SDL_Color color = items[i].selectable ? white : black;
        drawTextCentered(renderer, items[i].text, centerX, y, scale, color);
        if (static_cast<int>(i) == selected) {
            const int x = centerX - textWidth(items[i].text, scale) / 2 - 60;
            drawText(renderer, ">", x, y, scale, white);
        }
    }
}
