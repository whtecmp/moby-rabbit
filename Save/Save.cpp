#include "Save.h"
#include <algorithm>
#include <fstream>
#include <cstdlib>
using namespace std;

static string trim(string s) {
    s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
    return s;
}

SaveData loadSave(const string& path) {
    SaveData data;
    ifstream in(path);
    if (!in) {
        return data;
    }
    string line;
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == string::npos) {
            continue;
        }
        const string key = trim(line.substr(0, eq));
        const string value = trim(line.substr(eq + 1));
        if (key == "unlocked_level") {
            data.unlockedLevel = atoi(value.c_str());
        } else if (key == "fullscreen") {
            data.fullscreen = value == "1" || value == "true";
        } else if (key == "controls") {
            data.gamepad = value == "gamepad";
        }
    }
    data.unlockedLevel = clamp(data.unlockedLevel, 1, maxLevel);
    return data;
}

void writeSave(const string& path, const SaveData& data) {
    ofstream out(path, ios::trunc);
    if (!out) {
        return;
    }
    out << "# moby-rabbit save file (edit by hand if you like)\n";
    out << "unlocked_level=" << clamp(data.unlockedLevel, 1, maxLevel) << "\n";
    out << "fullscreen=" << (data.fullscreen ? 1 : 0) << "\n";
    out << "controls=" << (data.gamepad ? "gamepad" : "keyboard") << "\n";
}

int loadUnlockedLevel(const string& path) {
    return loadSave(path).unlockedLevel;
}

void saveUnlockedLevel(const string& path, int unlockedLevel) {
    SaveData data = loadSave(path);
    data.unlockedLevel = unlockedLevel;
    writeSave(path, data);
}
