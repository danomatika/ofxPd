/*
 *  ofxPdListener.h
 *  ofxPd
 *
 *  Created by Marek Bereza on 24/01/2011.
 *
 */
#pragma once

class ofxPdListener {

	public:
	
		void printReceived(string message) {}
		void noteonReceived(int channel, int pitch, int velocity) {}
		void controlChangeReceived(int channel, int controller, int val) {}
		void programChangeReceived(int channel, int program) {}
		void pitchbendReceived(int channel, int val) {}
		void aftertouchReceived(int channel, int val) {}
		void polyAftertouchReceived(int channel, int pitch, int val) {}
		void midibyteReceived(int port, int byte) {}
};
