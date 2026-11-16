//
// Created by user on 19/02/2026.
//

#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H
#include "Actor/Actor.h"
#include "Actor/Actors/Level/Level.h"
#include "State/State.h"
#include "Config/Config.h"
using namespace std;


// The player-controlled pirate mouse: runs left/right, jumps and attacks.
// Keyboard (arrows or A/D, Space/Up/W jump, X/Z attack) and gamepad
// (D-pad, A jump, X/B attack) both work at the same time.
// position is the top-left of the 128x128 idle sprite box.
class Player : public Actor {
    public:
    Player(const string& name, const Vector& startPosition, Level* level, Error& err);
    ~Player();

    void startJump();
    void startAttack();
    void update();
    // Sword hitbox in world coordinates; false when not attacking.
    bool attackBox(SDL_Rect& box) const;
    // Where the body can be hit, in world coordinates.
    SDL_Rect bodyBox() const;
    // Loses one hit point (ignored while briefly invulnerable afterwards).
    void takeHit();
    void drawHealth() const;
    void updateCamera(int worldWidth);
    void setAnimation(const string& name);

    ControlMode controlMode;
    SDL_JoystickID controllerId;
    Level* level;

    double speed;
    double jumpSpeed;
    double gravity;
    bool leftHeld;
    bool rightHeld;
    bool onGround;
    bool facingRight;
    bool attacking;
    Uint32 attackStartMs;
    double prevFeetY;
    int maxHp;
    int hp;
    Uint32 invulnerableUntilMs;
    Vector startPosition;
};


#endif //GAME_PLAYER_H
