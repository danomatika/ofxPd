#pragma once

#include "ofMain.h"

#include "ofxPd.h"

class AppCore : public ofxPdListener {

	public:

		// main
		void setup(const int numInChannels, const int numOutChannels,
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
		
		ofxPd pd;
};
