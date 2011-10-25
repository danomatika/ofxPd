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

#include "PdTypes.h"

namespace pd {

/// a pd message receiver base class
class PdReceiver {

	public:
	
		/// print
		virtual void receivePrint(const std::string& message) {};
		
		/// messages
		virtual void receiveBang(const std::string& dest) {}
		virtual void receiveFloat(const std::string& dest, float value) {}
		virtual void receiveSymbol(const std::string& dest, const std::string& symbol) {}
		virtual void receiveList(const std::string& dest, const List& list) {}
		virtual void receiveMessage(const std::string& dest, const std::string& msg, const List& list) {}

		/// midi
		virtual void receiveNote(const int channel, const int pitch, const int velocity) {}
		virtual void receiveCtl(const int channel, const int controller, const int value) {}
		virtual void receivePgm(const int channel, const int value) {} // note: pgm value is 1-128
		virtual void receiveBend(const int channel, const int value) {}
		virtual void receiveTouch(const int channel, const int value) {}
		virtual void receivePolyTouch(const int channel, const int pitch, const int value) {}
		
		/// raw midi byte
		virtual void receiveMidiByte(const int port, const int byte) {}
};

} // namespace
