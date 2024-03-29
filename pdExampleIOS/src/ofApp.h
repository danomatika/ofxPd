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
#include "ofxiOS.h"
#include "ofxiOSExtras.h"

#include "ofxPd.h"

// a namespace for the Pd types
using namespace pd;

// derive from Pd receiver classes to receive message and midi events
class ofApp : public ofxiOSApp, public PdReceiver, public PdMidiReceiver {

	public:

		// main
		void setup();
		void update();
		void draw();
		void exit();

		// input callbacks
		void keyPressed(int key);
	
		void touchDown(ofTouchEventArgs &touch);
		void touchMoved(ofTouchEventArgs &touch);
		void touchUp(ofTouchEventArgs &touch);
		void touchDoubleTap(ofTouchEventArgs &touch);
		void touchCancelled(ofTouchEventArgs &touch);

		void lostFocus();
		void gotFocus();
		void gotMemoryWarning();
		void deviceOrientationChanged(int newOrientation);
		
		// audio callbacks
		void audioIn(ofSoundBuffer &buffer);
		void audioOut(ofSoundBuffer &buffer);
		
		// pd message receiver callbacks
		void print(const std::string &message);
		
		void receiveBang(const std::string &dest);
		void receiveFloat(const std::string &dest, float value);
		void receiveSymbol(const std::string &dest, const std::string &symbol);
		void receiveList(const std::string &dest, const pd::List &list);
		void receiveMessage(const std::string &dest, const std::string &msg, const pd::List &list);
		
		// pd midi receiver callbacks
		void receiveNoteOn(const int channel, const int pitch, const int velocity);
		void receiveControlChange(const int channel, const int controller, const int value);
		void receiveProgramChange(const int channel, const int value);
		void receivePitchBend(const int channel, const int value);
		void receiveAftertouch(const int channel, const int value);
		void receivePolyAftertouch(const int channel, const int pitch, const int value);
		
		void receiveMidiByte(const int port, const int byte);
	
		// do something
		void playTone(int pitch);
	
		// sets the preferred sample rate, returns the *actual* samplerate
		// which may be different ie. iPhone 6S only wants 48k
		float setAVSessionSampleRate(float preferredSampleRate);
		
		ofxPd pd;
		std::vector<float> scopeArray;
		std::vector<pd::Patch> instances;
		
        int midiChan;
};
