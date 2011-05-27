#pragma once

#include <map>
#include <set>
#include <z_libpd.h>

#include "ofxPdListener.h"

#ifndef HAVE_UNISTD_H
#pragma warning You need to define HAVE_UNISTD_H in your project build settings!
#endif

///
///	a Pure Data instance
///
///	references:	http://gitorious.org/pdlib/pages/Libpd
///
class ofxPd {
	
	public :

		ofxPd();
		virtual ~ofxPd();
        
        /// initialize resources
		bool init(const int numOutChannels, const int numInChannels,
				  const int sampleRate, const int ticksPerBuffer=32);
        
        /// clear resources
        void clear();
		
		/// add to the pd search path
		void addToSearchPath(const std::string& path);
		
		/// clear the current pd search path
		void clearSearchPath();
		
		/// open a patch, take a absolute or relative path (in data folder)
		void openPatch(const std::string& patch);
		
		/// close a patch, takes the patch's basename (filename without extension)
		void closePatch(const std::string& name);
		
		// turn on/off digital signal processing
		void dspOn();
		void dspOff();
		
		//// \section Receiving
		
		/// add/remove listener to receive events
		///
		/// listeners automatically receive print and midi events only,
		/// use subscribe() to plug a listener into a source
		///
		void addListener(ofxPdListener& listener);
		void removeListener(ofxPdListener& listener);
		bool listenerExists(ofxPdListener& listener);
		void clearListeners();	/// also unsubscribes all listeners
		
		/// add/remove receiver sources from libpd
		///
		/// aka the pd receive name
		///
		/// [r source]
		/// |
		///
		/// note: the global source (aka "") exists by default 
		///
		void addSource(const std::string& source);
		void removeSource(const std::string& source);
		bool sourceExists(const std::string& source);
		void clearSources();	/// listeners will be unsubscribed from *all* sources
		
		/// un/subscribe a listener to a receiver source from libpd
		///
		/// un/subscribe using a source name or "" for all sources,
		/// make sure to add the listener and source first
		///
		/// note: the global source (aka "") is added by default
		/// note: unsubscribing from the global source unsubscribes from *all*
		///       sources, so the listener will not recieve any message events,
		///		  but still get print and midi events
		///
		/// also: use negation if you want to subscribe to all sources but one:
		///
		/// pd.subscribe(listener);				// subscribe to all
		/// pd.unsubscribe(listener, "source"); // unsubscribe from "source"
		///
		void subscribe(ofxPdListener& listener, const std::string& source="");
		void unsubscribe(ofxPdListener& listener, const std::string& source="");
		bool isSubscribed(ofxPdListener& listener, const std::string& source="");
		
		/// \section Sending Functions
		
		/// messages
		void sendBang(const std::string& dest);
		void sendFloat(const std::string& dest, float value);
		void sendSymbol(const std::string& dest, const std::string& symbol);
		
		/// compound messages
		void startList(const std::string& dest);
		void startMessage(const std::string& dest, const std::string& msg);
		void addFloat(const float value);
		void addSymbol(const std::string& symbol);
		void finish();
		
		/// midi
		void sendNote(const int pitch, const int velocity, const int channel=0);
		void sendControlChange(const int controller, const int value, const int channel=0);
		void sendProgramChange(const int value, const int channel=0);
		void sendPitchBend(const int value, const int channel=0);
		void sendAftertouch(const int value, const int channel=0);
		void sendPolyAftertouch(const int pitch, const int value, const int channel=0);		
		
		/// raw midi bytes
		///
		/// port is the pd midi out port, dev# * 16 + channel (0-15)
		/// so chan 3 on midi out dev #2 is 16 + 3 = 19
		///
		void sendMidiByte(const int value, const int port=0);
		void sendSysExByte(const int value, const int port=0);
		void sendSysRealtimeByte(const int value, const int port=0);
		
		/// \section Sending Stream Interface
		
		/// single messages
		ofxPd& operator<<(const Bang& var);
		ofxPd& operator<<(const Float& var);
		ofxPd& operator<<(const Symbol& var);
		
		/// compound messages
		ofxPd& operator<<(const StartList& var);
		ofxPd& operator<<(const StartMessage& var);
        
		/// add a float to the message
		ofxPd& operator<<(const bool var);
        ofxPd& operator<<(const int var);
        ofxPd& operator<<(const float var);
        ofxPd& operator<<(const double var);
        
		/// add a symbol to the message
		ofxPd& operator<<(const char var);
        ofxPd& operator<<(const char* var);
        ofxPd& operator<<(const std::string& var);
		
		/// midi
		ofxPd& operator<<(const Note& var);
		ofxPd& operator<<(const ControlChange& var);
		ofxPd& operator<<(const ProgramChange& var);
		ofxPd& operator<<(const PitchBend& var);
		ofxPd& operator<<(const Aftertouch& var);
		ofxPd& operator<<(const PolyAftertouch& var);
		
		/// compound raw midi bytes
		ofxPd& operator<<(const StartMidi& var);
		ofxPd& operator<<(const StartSysEx& var);
		ofxPd& operator<<(const StartSysRealtime& var);
		
		/// finish a compound message
        ofxPd& operator<<(const Finish& var);
		
		/// is a message currently in progress?
        inline bool isMsgInProgress() {return bMsgInProgress;}
		
		/// \section Array Access
		
		/// get pd array length
		int getArrayLen(const std::string& arrayName);
		
		/// read from a pd array
		bool readArray(const std::string& arrayName, std::vector<float>& dest, int readLen=-1, int offset=0);
		
		/// write to a pd array
		bool writeArray(const std::string& arrayName, std::vector<float>& source, int writeLen=-1, int offset=0);
		
		/// \section Utils
		
		/// get the blocksize of pd (sample length per channel)
		static int getBlockSize();
		
		/// sections Audio Processing Callbacks
		
		/// the libpd processing is done in the audioOut callback
		virtual void audioIn(float * input, int bufferSize, int nChannels);
		virtual void audioOut(float * output, int bufferSize, int nChannels);
		
    private:
	
		bool bPdInited;						///< is pd inited?

		int sampleRate;						///< the audio sample rate
		int numInChannels, numOutChannels;	///< number of channels in/out
		float *inputBuffer;  				///< interleaved input audio buffer
		
		bool bMsgInProgress;				///< is a compound message being constructed?
		
		/// compound message status
		enum MsgType {
			LIST,
			MSG,
			MIDI,
			SYSEX,
			SYSRT
		} msgType;
		
		std::string msgDest;	///< message destination
		std::string msgMsg;		///< message target message
		int midiPort;			///< target midi port
	
		/// a receiving sources's pointer and bound listeners
		struct Source {
			
			// data
			void* pointer;						///< source pointer
			std::set<ofxPdListener*> listeners;	///< subscribed listeners

			// helper functions
			void addListener(ofxPdListener* listener) {
				listeners.insert(listener);
			}
			
			void removeListener(ofxPdListener* listener) {
				std::set<ofxPdListener*>::iterator iter;
				iter = listeners.find(listener);
				if(iter != listeners.end())
					listeners.erase(iter);
			}

			bool listenerExists(ofxPdListener* listener) {
				if(listeners.find(listener) != listeners.end())
					return true;
				return false;
			}
		};
			
		std::set<ofxPdListener*> listeners;		///< the listeners
		std::map<std::string,Source> sources;	///< bound sources
												///< first object always global
		
		std::string printMsg;	///< used to build a print message
		
		
		// libpd static callback functions
		static void _print(const char* s);
				
		static void _bang(const char* source);
		static void _float(const char* source, float value);
		static void _symbol(const char* source, const char* symbol);
		
		static void _list(const char* source, int argc, t_atom* argv); 
		static void _message(const char* source, const char *symbol,
												int argc, t_atom *argv);

		static void _noteon(int channel, int pitch, int velocity);
		static void _controlchange(int channel, int controller, int value);
		static void _programchange(int channel, int value);
		static void _pitchbend(int channel, int value);
		static void _aftertouch(int channel, int value);
		static void _polyaftertouch(int channel, int pitch, int value);
		
		static void _midibyte(int port, int byte);
};
