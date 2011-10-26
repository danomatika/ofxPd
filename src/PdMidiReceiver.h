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

namespace pd {

/// a pd midi receiver base class
class PdMidiReceiver {

	public:

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
