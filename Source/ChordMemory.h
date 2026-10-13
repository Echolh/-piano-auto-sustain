#pragma once
#include <cstdint>

// A deliberately conservative left-hand heuristic, not a score/chord recogniser.
// Remember released chord tones so broken accompaniment does not become a series
// of false bass changes. Upper melody notes never enter this memory.
struct ChordMemory {
    using Mask=std::uint16_t;
    Mask notes=0;
    int root=-1,lastBass=-1;
    static Mask bit(int note) {return static_cast<Mask>(1u<<(note%12));}
    static Mask transpose(Mask mask,int root) {return static_cast<Mask>(((mask<<root)|(mask>>(12-root)))&4095);}
    static int count(Mask mask) {int n=0; for(;mask;mask=static_cast<Mask>(mask&(mask-1))) ++n; return n;}
    static constexpr Mask triads[]={145,137,73,273}; // major, minor, diminished, augmented
    static constexpr Mask complete[]={145,137,73,273,133,161,1169,2193,1161,1097,585,2185};
    static int knownRoot(Mask mask,int preferred) {
        if(preferred>=0) for(auto chord:complete) if(transpose(chord,preferred)==mask) return preferred;
        for(int r=0;r<12;++r) for(auto chord:complete) if(transpose(chord,r)==mask) return r;
        return -1;
    }
    void reset(Mask mask=0,int bass=-1) {
        notes=mask; lastBass=bass;
        root=mask?knownRoot(mask,bass%12):-1;
        if(root<0 && bass>=0) root=bass%12;
    }
    // Returns true when the new group needs a pedal change. A rejected group
    // does not overwrite the old context until that change actually happens.
    bool needsChange(Mask incoming,int bass,bool octaveSame) {
        if(!notes) {reset(incoming,bass); return false;}
        if(!octaveSame && lastBass>=0 && incoming==bit(lastBass) && bass%12==lastBass%12 && bass!=lastBass) return true;
        const auto joined=static_cast<Mask>(notes|incoming);
        const int oldRoot=knownRoot(notes,root),newRoot=knownRoot(incoming,bass%12);
        // A newly played complete voicing is stronger evidence than an individual
        // common tone. E-G-B after C-E-G must not be absorbed into Cmaj7.
        if(oldRoot>=0 && newRoot>=0 && incoming!=notes && (incoming&~notes)!=0) return true;
        bool compatible=(incoming&~notes)==0;
        if(!compatible && oldRoot<0) {
            // Do not explain away a new F or D bass after C-G by inventing a
            // suspended chord. Suspensions must be played as a complete group.
            for(int r=0;r<12;++r) for(auto chord:triads) if(transpose(chord,r)==joined) compatible=true;
            if(newRoot>=0 && (notes&~incoming)==0) compatible=true;
        }
        if(!compatible && root>=0) {
            if(oldRoot<0) {
                for(auto chord:triads) if((joined&~transpose(chord,root))==0) compatible=true;
            } else {
                // Allow a root-consistent seventh extension, not arbitrary growth
                // into every chord that happens to contain the previous triad.
                for(auto chord:complete) if(count(chord)==4 && (joined&~transpose(chord,root))==0) compatible=true;
            }
        }
        if(!compatible) return true;
        notes=joined; lastBass=bass;
        if(const int r=knownRoot(notes,root); r>=0) root=r;
        return false;
    }
};
