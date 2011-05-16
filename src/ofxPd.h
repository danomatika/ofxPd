#pragma once

#include <z_libpd.h>

#include "ofxPdListener.h"

#ifndef HAVE_UNISTD_H
#pragma warning You need to define HAVE_UNISTD_H in your project build settings!
#endif

///
///	a Pure Data instance
///
/// derive this class and implement the callback functions
///
///	references:	http://gitorious.org/pdlib/pages/Libpd
///
class ofxPd {
	
	public :

		ofxPd();
		virtual ~ofxPd();
        
        /// initialize resources
		bool init(const int numInChannels, const int numOutChannels, 
					const int sampleRate);
        
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
		
		/** \section Send Functions */
		
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
		void sendMidiByte(const int value, const int port=0);
		void sendSysExByte(const int value, const int port=0);
		void sendSysRealtimeByte(const int value, const int port=0);
		
		/** \section Send Stream Interface */
		
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
		
		/// is a stream message currently in progress?
        inline bool isMsgInProgress() {return bMsgInProgress;}
	
		
			
		
		
		/// add listener to receieve events
		void addListener(ofxPdListener& listener, const std::string& source="");
		void addListener(ofxPdListener& listener, const std::vector<std::string>& sources);
		
		/// add message source names to receive to
		void bind(ofxPdListener& listener, const std::string& source);
		void unbind(ofxPdListener& listener, const std::string& source);
		
		
		
		/// get the pd blocksize of pd (sample length per channel)
		static int getBlocksize();
		
		/// audio in/out callbacks
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
