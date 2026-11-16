#include "Save.h"
#include <algorithm>
#include <fstream>
#include <cstdlib>
using namespace std;

int loadUnlockedLevel(const string& path) {
    ifstream in(path);
    if (!in) {
        return 1;
    }
    int unlocked = 1;
    string line;
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == string::npos) {
            continue;
        }
        string key = line.substr(0, eq);
        key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());
        if (key == "unlocked_level") {
            unlocked = atoi(line.c_str() + eq + 1);
        }
    }
    return clamp(unlocked, 1, maxLevel);
}

void saveUnlockedLevel(const string& path, int unlockedLevel) {
    ofstream out(path, ios::trunc);
    if (!out) {
        return;
    }
    out << "# moby-rabbit save file (edit by hand if you like)\n";
    out << "unlocked_level=" << clamp(unlockedLevel, 1, maxLevel) << "\n";
}
