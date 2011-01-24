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
	void pdPrintReceived(string message) {}
	void pdNoteonReceived(int channel, int pitch, int velocity) {}
	void pdControlChangeReceived(int channel, int controller, int val) {}
	void pdProgramChangeReceived(int channel, int program) {}
	void pdPitchbendReceived(int channel, int val) {}
	void pdAftertouchReceived(int channel, int val) {}
	void pdPolyAftertouchReceived(int channel, int pitch, int val) {}
};
