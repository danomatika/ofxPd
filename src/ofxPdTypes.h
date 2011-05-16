#pragma once

#include <string>

///
/// ofxPd stream interface message objects
///

/// bang event
struct Bang {

	const std::string dest; ///< dest receiver name
	
	explicit Bang(const std::string& dest) : dest(dest) {}
};

/// float value
struct Float : public Bang {

	const float value;
	
	explicit Float(const std::string& dest, const float value) :
		Bang(dest), value(value) {}
};

/// symbol value
struct Symbol : public Bang {

	const std::string symbol;

	explicit Symbol(const std::string& dest, const std::string& symbol) :
		Bang(dest), symbol(symbol) {}
};

class ofxPd;

// a received compound message
class List {

	public:

		const unsigned int len();	// number of items
		const std::string& types();	// OSC style type string ie "fssfffs"

		// check type
		const bool isFloat(const unsigned int index);
		const bool isSymbol(const unsigned int index);

		// get item as type
		const float asFloat(const unsigned int index);
		const std::string& asSymbol(const unsigned int index);

	protected:

		List(const std::string& dest, const unsigned int length, const t_atom *items);

	private:

		std::string typeString;
		const unsigned int length;
		const t_atom *items;
};

/// start a list
struct StartList : public Bang {
    explicit
		StartList(const std::string& dest) : Bang(dest) {}
};

/// start a typed message
struct StartMessage : public Bang {

	const std::string msg; ///< target msg at the dest

    explicit
		StartMessage(const std::string& dest, const std::string& msg) :
			Bang(dest), msg(msg) {}
};

///
/// ofxPd stream interface midi objects
/// ref: http://www.gweep.net/~prefect/eng/reference/protocol/midispec.html
///

/// send a note on/off event (set vel = 0 for noteoff)
struct Note {

	const int channel;	///< hannel (0-15)
	const int pitch;	///< pitch (0-127)
	const int velocity;	///< velocity (0-127)
	
	explicit Note(const int pitch, const int velocity=64, const int channel=0) :
		pitch(pitch), velocity(velocity), channel(channel) {}
};

/// change a control value aka send a CC message
struct ControlChange {

	const int channel;		///< channel (0-15)
	const int controller;	///< controller (0-127)
	const int value;		///< value (0-127)
	
	explicit ControlChange(const int controller, const int value, const int channel=0) :
		controller(controller), value(value), channel(channel) {}
};

/// change a program value (ie an instrument)
struct ProgramChange {

	const int channel;	///< channel (0-15)
	const int value;	///< value (0-127)
	
	explicit ProgramChange(const int value, const int channel=0) :
		value(value), channel(channel) {}
};

/// change the pitch bend value
struct PitchBend {

	const int channel;	///< channel (0-15)
	const int value;	///< value (-8192 to 8192)
	
	explicit PitchBend(const int value, const int channel=0) :
		value(value), channel(channel) {}
};

/// change an aftertouch value
struct Aftertouch {

	const int channel;	///< channel (0-15)
	const int value;	///< value (0-127)
	
	explicit Aftertouch(const int value, const int channel=0) :
		value(value), channel(channel) {}
};

/// change a poly aftertouch value
struct PolyAftertouch {

	const int channel;	///< channel (0-15)
	const int pitch;	///< controller (0-127)
	const int value;	///< value (0-127)
	
	explicit PolyAftertouch(const int pitch, const int value, const int channel=0) :
		pitch(pitch), value(value), channel(channel) {}
};

/// start a raw midi byte stream
struct StartMidi {

	const int port; 	///< pd midi port
	
	explicit StartMidi(const int port=0) : port(port) {}
};

/// start a raw sysex byte stream
struct StartSysEx {

	const int port; 	///< pd midi port
	
	explicit StartSysEx(const int port=0) : port(port) {}
};

/// start a sys realtime byte stream
struct StartSysRealtime {

	const int port; 	///< pd midi port
	
	explicit StartSysRealtime(const int port=0) : port(port) {}
};

///
/// ofxPd stream interface endcap
///

/// finish the current compound message
struct Finish {
	explicit Finish() {}
};
