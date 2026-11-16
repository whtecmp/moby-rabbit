#ifndef GAME_BUG_H
#define GAME_BUG_H
#include "Actor/Actor.h"
#include "Actor/Actors/Player/Player.h"
using namespace std;


// Simple enemy that patrols left and right between two x positions.
// One hit from the player's sword kills it; touching the player costs him a hit point.
// There is no bug artwork wired in yet, so it is drawn as a placeholder box.
// position is the top-left of the box.
class Bug : public Actor {
    public:
    Bug(const string& name, double minX, double maxX, double feetY, Player* player, Error& err);
    ~Bug();

    void update();
    void draw() const;

    Player* player;
    double minX;
    double maxX;
    double speed;
    bool movingRight;
    bool alive;
    int width;
    int height;
};


#endif //GAME_BUG_H
