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

#include "ofxPd.h"

class AppCore : public ofxPdListener {

	public:

		// main
		void setup(const int numOutChannels, const int numInChannels,
				   const int sampleRate, const int ticksPerBuffer);
		void update();
		void draw();
        void exit();

		// do something
		void playTone(int pitch);
		
		// input callbacks
		void keyPressed(int key);
		
		// audio callbacks
		void audioReceived(float * input, int bufferSize, int nChannels);
		void audioRequested(float * output, int bufferSize, int nChannels);
		
		// pd callbacks
		void printReceived(const std::string& message);
		
		void bangReceived(const std::string& dest);
		void floatReceived(const std::string& dest, float value);
		void symbolReceived(const std::string& dest, const std::string& symbol);
		void listReceived(const std::string& dest, const List& list);
		void messageReceived(const std::string& dest, const std::string& msg, const List& list);
		
		void noteReceived(const int channel, const int pitch, const int velocity);
		void ctlReceived(const int channel, const int controller, const int value);
		void pgmReceived(const int channel, const int value);
		void bendReceived(const int channel, const int value);
		void touchReceived(const int channel, const int value);
		void polyTouchReceived(const int channel, const int pitch, const int value);
		
		void midiByteReceived(const int port, const int byte);
		
		ofxPd pd;
};
