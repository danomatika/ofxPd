#pragma once

#include "ofMain.h"

/// \section ofxPd Patch

class Patch {

	public:

		Patch();
		Patch(void* handle, int dollarZero, const std::string& filename, const std::string& path);
			
		/// data access
		void* handle()				const	{return _handle;}
		int dollarZero()			const	{return _dollarZero;}
		std::string filename()		const	{return _filename;}
		std::string path()			const	{return _path;}
		
		/// get dollarZero as a string
		std::string dollarZeroStr()	const	{return _dollarZeroStr;}
			
		/// is the patch pointer valid?
		bool isValid() const;
		
		/// clear data in this object (does not close patch!)
		void clear();

		/// copy constructor
		Patch(const Patch& from);

        /// copy operator
        void operator=(const Patch& from);
		
		/// print to ostream
		friend std::ostream& operator<<(std::ostream& os, const Patch& from);

	private:
	
		void* _handle;				///< patch handle pointer
		int _dollarZero;			///< the unique patch id, ie $0
		std::string _dollarZeroStr;	///< $0 as a string
		
		std::string _filename;	///< filename
		std::string _path;		///< full path
};

/// \section ofxPd message types

enum ofxPdMsgType {
	OFX_PD_BANG,
	OFX_PD_FLOAT,
	OFX_PD_SYMBOL,
	OFX_PD_LIST,
	OFX_PD_MESSAGE
};

/// \section ofxPd stream interface message objects

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

/// list, a compound message
class List : public Bang {

	public:

		List(const std::string& dest) : Bang(dest) {}
	
		/// \section Read

		/// check type
		bool isFloat(const unsigned int index) const;
		bool isSymbol(const unsigned int index) const;

		/// get item as type
		float asFloat(const unsigned int index) const;
		std::string asSymbol(const unsigned int index) const;

		/// \section Write
		
		void addFloat(const float value);
		void addSymbol(const std::string& symbol);
		
		/// \section Util
		
		const unsigned int len() const;		///< number of items
		const std::string& types() const;	///< OSC style type string ie "fsfs"
		void clear(); 						///< clear all objects

		/// get list as a string
		std::string toString() const;

		/// print to ostream
		friend std::ostream& operator<<(std::ostream& os, const List& from);

	private:

		std::string typeString;	///< OSC style type string
		
		// object wrapper
		struct MsgObject {
			ofxPdMsgType type;
			float value;
			std::string symbol;
		};
		
		std::vector<MsgObject> objects;	///< list objects
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

/// /section ofxPd stream interface midi objects
/// ref: http://www.gweep.net/~prefect/eng/reference/protocol/midispec.html

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

	const int port; 	///< pd midi port, dev# * 16 + channel (0-15)
						/// so dev #2, chan 3 = 16 + 3 = 19
	
	explicit StartMidi(const int port=0) : port(port) {}
};

/// start a raw sysex byte stream
struct StartSysEx {

	const int port; 	///< pd midi port, dev# * 16 + channel (0-15)
						/// so dev #2, chan 3 = 16 + 3 = 19
	
	explicit StartSysEx(const int port=0) : port(port) {}
};

/// start a sys realtime byte stream
struct StartSysRealtime {

	const int port; 	///< pd midi port, dev# * 16 + channel (0-15)
						/// so dev #2, chan 3 = 16 + 3 = 19
	
	explicit StartSysRealtime(const int port=0) : port(port) {}
};

///
/// ofxPd stream interface endcap
///

/// finish the current compound message
struct Finish {
	explicit Finish() {}
};
