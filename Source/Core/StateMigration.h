#pragma once
#include "SustainSettings.h"
#include <algorithm>

namespace pas {
struct StateMigration {
    template<class Map>
    static SustainSettings fromVersionedMap(int schemaVersion, const Map& values) {
        SustainSettings result;
        auto number = [&](const char* key, double fallback) {
            const auto it = values.find(key);
            return it == values.end() ? fallback : static_cast<double>(it->second);
        };
        auto integer = [&](const char* key, int fallback, int low, int high) {
            return std::clamp(static_cast<int>(number(key, fallback)), low, high);
        };
        auto boolean = [&](const char* key, bool fallback) {
            return number(key, fallback ? 1.0 : 0.0) >= 0.5;
        };

        result.split = integer("split", result.split, 24, 84);
        result.groupMs = integer("group", result.groupMs, 5, 200);
        result.gapMs = integer("gap", result.gapMs, 1, 250);
        result.guardMs = integer("guard", result.guardMs, 0, 2000);
        result.tailMs = integer("tail", result.tailMs, 100, 10000);
        result.enabled = boolean("enabled", result.enabled);
        result.octaveSame = boolean("octave", result.octaveSame);
        result.chordProtection = boolean("chordProtection", result.chordProtection);
        result.noteHold = boolean("noteHold", result.noteHold);

        const auto legacyMode = values.find("mode");
        if (schemaVersion <= 1 && legacyMode != values.end())
            result.noteHold = static_cast<double>(legacyMode->second) < 0.5;
        const auto legacyStrategy = values.find("strategy");
        if (schemaVersion <= 1 && legacyStrategy != values.end())
            result.chordProtection = static_cast<double>(legacyStrategy->second) < 0.5;
        return result;
    }
};
}

