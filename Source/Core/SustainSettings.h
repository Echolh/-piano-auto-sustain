#pragma once

namespace pas {
struct SustainSettings {
    int split = 59;
    int groupMs = 40;
    int gapMs = 35;
    int tailMs = 2400;
    int guardMs = 160;
    bool noteHold = false;
    bool enabled = true;
    bool octaveSame = true;
    bool chordProtection = true;
};
}

