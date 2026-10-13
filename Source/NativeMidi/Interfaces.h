#pragma once
// Use the current, Unicode-capable factory ABI. The MIDI-only interfaces have
// no strings. Their compatibility with Cubase 15 still requires a host test.
#include "pluginterfaces/base/ipluginbase.h"
using namespace Steinberg;
#define __ipluginbase__
#include "pluginterfaces/midi/imidieffect.h"
#undef __ipluginbase__
#define __funknown__
#include "pluginterfaces/gui/iplugparams.h"
#include "pluginterfaces/gui/iplugui.h"
#include "pluginterfaces/base/iplugstorage.h"
#undef __funknown__

#ifdef BASS_PEDAL_NATIVE_SUSTAIN
inline constexpr TUID BassPedalProbeID = INLINE_UID(0x907880BC,0xBC874AD9,0x9CD13EA2,0x9270F069);
#else
inline constexpr TUID BassPedalProbeID = INLINE_UID(0xC09D0611,0x42464D24,0x92C60711,0x72EFD037);
#endif
