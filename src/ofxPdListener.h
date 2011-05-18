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
		virtual void printReceived(const std::string& message) {};
		
		/// messages
		virtual void bangReceived(const std::string& dest) {}
		virtual void floatReceived(const std::string& dest, float value) {}
		virtual void symbolReceived(const std::string& dest, const std::string& symbol) {}
		virtual void listReceived(const std::string& dest, const List& list) {}
		virtual void messageReceived(const std::string& dest, const std::string& msg, const List& list) {}
		
		/// midi
		virtual void noteReceived(const int channel, const int pitch, const int velocity) {}
		virtual void controlChangeReceived(const int channel, const int controller, const int value) {}
		virtual void programChangeReceived(const int channel, const int value) {}
		virtual void pitchBendReceived(const int channel, const int value) {}
		virtual void aftertouchReceived(const int channel, const int value) {}
		virtual void polyAftertouchReceived(const int channel, const int pitch, const int value) {}
		
		/// raw midi byte
		virtual void midiByteReceived(const int port, const int byte) {}
};
