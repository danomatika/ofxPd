/*
 * Copyright (c) 2011 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxPd for documentation
 *
 * This file created by Marek Bereza on 24/01/2011. Updated by Dan Wilcox 2011.
 *
 */
#pragma once

#include "ofxPdTypes.h"

/// a pd message receiver base class
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
		virtual void ctlReceived(const int channel, const int controller, const int value) {}
		virtual void pgmReceived(const int channel, const int value) {} // note: pgm value is 1-128
		virtual void bendReceived(const int channel, const int value) {}
		virtual void touchReceived(const int channel, const int value) {}
		virtual void polyTouchReceived(const int channel, const int pitch, const int value) {}
		
		/// raw midi byte
		virtual void midiByteReceived(const int port, const int byte) {}
};
