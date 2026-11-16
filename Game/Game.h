#ifndef GAME_GAME_H
#define GAME_GAME_H
#include "Actor/Actor.h"
#include "Actor/Actors/Level/Level.h"
#include "Actor/Actors/Player/Player.h"
#include <string>
#include <vector>
using namespace std;


// Ties the screens together: main menu, level selection and playing a level.
// Menus are drawn with the built-in pixel font. Selectable text is white and
// text that cannot be selected is black.
class Game : public Actor {
    public:
    enum Mode { mainMenu, levelSelect, playing, paused };

    struct MenuItem {
        string text;
        bool selectable;
    };

    Game(Level* level, Player* player, Error& err);
    ~Game();

    // Starts (or restarts) level number 1-10 with fresh enemies and full health.
    void startLevel(int number);
    void showMainMenu();
    void showLevelSelect();
    void pause();
    void resume();
    void toggleControls();
    void toggleFullscreen();
    bool fullscreen() const;
    void saveSettings() const;

    vector<MenuItem> menuItems() const;
    void moveSelection(int step);
    void activate();
    void back();
    void drawOverlay() const;
    // Called every frame; finishes the level when the player reaches the goal.
    void update();
    void finishLevel();

    Level* level;
    Player* player;
    Mode mode;
    int selected;
    int unlocked;
    int currentLevel;
    int bugCounter;
    vector<string> bugNames;
    string savePath;
};


#endif //GAME_GAME_H
