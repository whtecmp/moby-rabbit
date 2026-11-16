#ifndef GAME_SAVE_H
#define GAME_SAVE_H
#include <string>
using namespace std;

// The save file is a plain text file that can be edited by hand:
//   # comment lines start with '#'
//   unlocked_level=3
// Levels are numbered 1 to maxLevel; level 1 is always unlocked.
const int maxLevel = 10;

// Highest unlocked level; 1 when the file is missing or unreadable.
int loadUnlockedLevel(const string& path);
void saveUnlockedLevel(const string& path, int unlockedLevel);

#endif //GAME_SAVE_H
