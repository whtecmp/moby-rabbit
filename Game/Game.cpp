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
    bind(Actor::controllerButtonDown, SDL_CONTROLLER_BUTTON_START, &Game::back);
    bind(Actor::controllerButtonDown, SDL_CONTROLLER_BUTTON_B, &Game::back);

    onRender = [this](Actor& actor) {
        static_cast<void>(actor);
        update();
    };
    // Menus are laid out for 1920x1080 whatever the window or screen size is.
    SDL_RenderSetLogicalSize(state.env->renderer->renderer, state.view->width, state.view->height);
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

void Game::update() {
    if (mode == playing && !State::get().paused && level->goalTouches(player->bodyBox())) {
        finishLevel();
    }
}

// Unlocks the next level in the save file and moves on to it.
void Game::finishLevel() {
    if (currentLevel >= unlocked && unlocked < maxLevel) {
        unlocked = currentLevel + 1;
        saveUnlockedLevel(savePath, unlocked);
    }
    if (currentLevel >= maxLevel) {
        showMainMenu();
        return;
    }
    startLevel(currentLevel + 1);
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
    state.paused = false;
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
    } else if (mode == paused) {
        items.push_back({"Resume Game", true});
        const bool gamepad = State::get().controlMode == GAMEPAD;
        items.push_back({string("Choose Controls: ") + (gamepad ? "Gamepad" : "Keyboard"), true});
        items.push_back({string("Display: ") + (fullscreen() ? "Borderless Fullscreen" : "Windowed"), true});
        items.push_back({"Go to Main Menu", true});
        items.push_back({"Exit Game", true});
    } else if (mode == levelSelect) {
        for (int i = 1; i <= maxLevel; ++i) {
            items.push_back({"Level " + to_string(i), i <= unlocked});
        }
        items.push_back({"Back", true});
    }
    return items;
}

bool Game::fullscreen() const {
    SDL_Window* window = State::get().env->window->window;
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

void Game::toggleFullscreen() {
    SDL_Window* window = State::get().env->window->window;
    SDL_SetWindowFullscreen(window, fullscreen() ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

// Switches the player between keyboard and gamepad control.
void Game::toggleControls() {
    State& state = State::get();
    if (state.controlMode == KEYBOARD) {
        if (state.controller == nullptr) {
            cerr << "No game controller found; staying on keyboard." << endl;
            return;
        }
        state.controlMode = GAMEPAD;
        state.controllerId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(state.controller));
    } else {
        state.controlMode = KEYBOARD;
    }
    player->controlMode = state.controlMode;
    player->controllerId = state.controllerId;
    player->leftHeld = false;
    player->rightHeld = false;
}

void Game::pause() {
    if (mode != playing) {
        return;
    }
    mode = paused;
    selected = 0;
    State::get().paused = true;
}

void Game::resume() {
    if (mode != paused) {
        return;
    }
    mode = playing;
    State::get().paused = false;
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
    } else if (mode == paused) {
        if (selected == 0) {
            resume();
        } else if (selected == 1) {
            toggleControls();
        } else if (selected == 2) {
            toggleFullscreen();
        } else if (selected == 3) {
            State::get().paused = false;
            showMainMenu();
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
    if (mode == playing) {
        pause();
    } else if (mode == paused) {
        resume();
    } else if (mode == levelSelect) {
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
    if (mode == paused) {
        // Solid black box with the options as white text.
        const int boxSize = 760;
        SDL_Rect box{centerX - boxSize / 2, (state.view->height - boxSize) / 2, boxSize, boxSize};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &box);
        const int scale = 4;
        const int rowHeight = textHeight(scale) + 36;
        const vector<MenuItem> items = menuItems();
        const int top = box.y + 120;
        drawTextCentered(renderer, "Paused", centerX, box.y + 40, 6, white);
        for (size_t i = 0; i < items.size(); ++i) {
            const int y = top + static_cast<int>(i) * rowHeight;
            drawTextCentered(renderer, items[i].text, centerX, y, scale, white);
            if (static_cast<int>(i) == selected) {
                drawText(renderer, ">", box.x + 40, y, scale, white);
            }
        }
        return;
    }
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
