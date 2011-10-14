/*
 * Copyright (c) 2011 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxPd for documentation
 *
 */
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

        List() : Bang("") {}
		List(const std::string& dest) : Bang(dest) {}
	
		/// \section Read

		/// check type
		bool isFloat(const unsigned int index) const;
		bool isSymbol(const unsigned int index) const;

		/// get item as type
		float asFloat(const unsigned int index) const;
		std::string asSymbol(const unsigned int index) const;

		/// \section Write
		
        /// add elements to the list
        ///
        /// List list;
        /// list.addSymbol("hello");
        /// list.addFloat(1.23);
		///
        void addFloat(const float value);
		void addSymbol(const std::string& symbol);
        
        /// \section Write Stream Interface
        
        /// list << "hello" << 1.23;
        
        /// add a float to the message
        List& operator<<(const bool var);
        List& operator<<(const int var);
        List& operator<<(const float var);
        List& operator<<(const double var);
        
        /// add a symbol to the message
        List& operator<<(const char var);
        List& operator<<(const char* var);
        List& operator<<(const std::string& var);
		
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
		
		// object type
		enum MsgType {
			FLOAT,
			SYMBOL
		};
		
		// object wrapper
		struct MsgObject {
			MsgType type;
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
struct StartMsg : public Bang {

	const std::string msg; ///< target msg at the dest

    explicit
		StartMsg(const std::string& dest, const std::string& msg) :
			Bang(dest), msg(msg) {}
};

/// /section ofxPd stream interface midi objects
/// ref: http://www.gweep.net/~prefect/eng/reference/protocol/midispec.html

/// send a note on/off event (set vel = 0 for noteoff)
struct Note {

	const int channel;	///< channel (1 - 16 * dev#)
	const int pitch;	///< pitch (0 - 127)
	const int velocity;	///< velocity (0 - 127)
	
	explicit Note(const int pitch, const int velocity=64, const int channel=1) :
		pitch(pitch), velocity(velocity), channel(channel) {}
};

/// change a control value aka send a CC message
struct Ctl {

	const int channel;		///< channel (1 - 16 * dev#)
	const int controller;	///< controller (0 - 127)
	const int value;		///< value (0 - 127)
	
	explicit Ctl(const int controller, const int value, const int channel=1) :
		controller(controller), value(value), channel(channel) {}
};

/// change a program value (ie an instrument)
struct Pgm {

	const int channel;	///< channel (1 - 16 * dev#)
	const int value;	///< value (0 - 127)
	
	explicit Pgm(const int value, const int channel=1) :
		value(value), channel(channel) {}
};

/// change the pitch bend value
struct Bend {

	const int channel;	///< channel (1 - 16 * dev#)
	const int value;	///< value (-8192 - 8192)
	
	explicit Bend(const int value, const int channel=1) :
		value(value), channel(channel) {}
};

/// change an aftertouch value
struct Touch {

	const int channel;	///< channel (1 - 16 * dev#)
	const int value;	///< value (0 - 127)
	
	explicit Touch(const int value, const int channel=1) :
		value(value), channel(channel) {}
};

/// change a poly aftertouch value
struct PolyTouch {

	const int channel;	///< channel (1 - 16 * dev#)
	const int pitch;	///< controller (0 - 127)
	const int value;	///< value (0 - 127)
	
	explicit PolyTouch(const int pitch, const int value, const int channel=1) :
		pitch(pitch), value(value), channel(channel) {}
};

/// start a raw midi byte stream
struct StartMidi {

	const int port; 	///< raw portmidi port, see http://en.wikipedia.org/wiki/PortMidi
	
	explicit StartMidi(const int port=0) : port(port) {}
};

/// start a raw sysex byte stream
struct StartSysEx {

	const int port; 	///< raw portmidi port
	
	explicit StartSysEx(const int port=0) : port(port) {}
};

/// start a sys realtime byte stream
struct StartSysRT {

	const int port; 	///< raw portmidi port
	
	explicit StartSysRT(const int port=0) : port(port) {}
};

///
/// ofxPd stream interface endcap
///

/// finish the current compound message
struct Finish {
	explicit Finish() {}
};
