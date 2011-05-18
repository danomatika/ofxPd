/*
 *  ofxPdListener.h
 *  ofxPd
 *
 *  Created by Marek Bereza on 24/01/2011.
 *
 */
#pragma once

#include "ofxPdTypes.h"

class ofxPdListener {

	public:
	
		/// print
		void printReceived(const std::string& message) {}
		
		/// messages
		void bangReceived(const std::string& dest) {}
		void floatReceived(const std::string& dest, float value) {}
		void symbolReceived(const std::string& dest, const std::string& symbol) {}
		void listReceived(const std::string& dest, const List& list) {}
		void messageReceived(const std::string& dest, const std::string& msg, const List& list) {}
		
		/// midi
		void noteReceived(const int channel, const int pitch, const int velocity) {}
		void controlChangeReceived(const int channel, const int controller, const int value) {}
		void programChangeReceived(const int channel, const int value) {}
		void pitchBendReceived(const int channel, const int value) {}
		void aftertouchReceived(const int channel, const int value) {}
		void polyAftertouchReceived(const int channel, const int pitch, const int value) {}
		
		/// raw midi byte
		void midiByteReceived(const int port, const int byte) {}
		
		
		
};
