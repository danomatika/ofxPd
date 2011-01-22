#ifndef _OFX_PD
#define _OFX_PD

#include "ofConstants.h"

#include <z_libpd.h>

///
///	a Pure Data instance
///
/// derive this class an implement the callback functions
///
class ofxPd
{

	public :

		ofxPd();
		virtual ~ofxPd();
        
        /// initialize resources, must be called before open()
		bool pdInit();
        
        /// clear resources
        void pdClear();
		
		/// process 1 audio frame (here for now, will probably remove at some point)
		void pdUpdate();
		
		/// add to the pd search path
		void pdAddToSearchPath(const string& path);
		
		/// clear the current pd search path
		void pdClearSearchPath();
		
		/// open a patch, take a absolute or relative path (in data folder)
		void pdOpenPatch(const string& patch);
		
		/// close a patch, takes the patch's basename (filename without extension)
		void pdClosePatch(const string& name);
		
		// turn on/off digital signal processing
		void pdDspOn();
		void pdDspOff();
		
		/// send messages to pd
		void pdSendFloat(const string& messageName, float value);
		void pdSendBang(const string& messageName);	
		void pdSendMidiNote(int channel, int noteNum, int velocity);
		void pdSendMidiControlChange(int channel, int ctlNum, int value);
		void pdSendMidiBend(int channel, int value);
		void pdSendMidiProgramChange(int channel, int program);
		void pdSendMidiPolyTouch(int channel, int noteNum, int value);
		void pdSendMidiAfterTouch(int channel, int value);
		
		// callbacks for you to implement
		void pdPrintReceived(const string& message) {}
		void pdNoteonReceived(int channel, int pitch, int velocity) {}
		void pdControlChangeReceived(int channel, int controller, int val) {}
		void pdProgramChangeReceived(int channel, int program) {}
		void pdPitchbendReceived(int channel, int val) {}
		void pdAftertouchReceived(int channel, int val) {}
		void pdPolyAftertouchReceived(int channel, int pitch, int val) {}

	protected:

		int srate;	///< the audio sample rate
		float inbuf[64], outbuf[128];  /// one input channel, two output channels
									   /// block size 64, one tick per buffer		
    private:
	
		bool	bPdInited;
		bool	bVerbose;
	
		// libpd static callback functions
		static void _print(const char* s);
		
		static void _noteon(int channel, int pitch, int velocity);
		static void _controlchange(int channel, int controller, int val);
		static void _programchange(int channel, int program);
		static void _pitchbend(int channel, int val);
		static void _aftertouch(int channel, int val);
		static void _polyaftertouch(int channel, int pitch, int val);
};

#endif
