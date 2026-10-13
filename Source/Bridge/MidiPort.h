#pragma once
#include "Core/MidiEvent.h"
#include <string>
namespace pas {
struct IMidiInput { virtual ~IMidiInput()=default; virtual std::string identifier() const=0; };
struct IMidiOutput { virtual ~IMidiOutput()=default; virtual std::string identifier() const=0; virtual bool send(MidiEvent)=0; };
}
