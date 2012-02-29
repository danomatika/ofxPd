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
#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup() {

	// register touch events
	ofRegisterTouchEvents(this);
	
	// initialize the accelerometer
	ofxAccelerometer.setup();
	
	// iPhoneAlerts will be sent to this
	ofxiPhoneAlerts.addListener(this);
	
	// if you want a landscape orientation 
	// ofxiPhoneSetOrientation(OFXIPHONE_ORIENTATION_LANDSCAPE_RIGHT);
	
	ofBackground(127, 127, 127);
	
	// the number if libpd ticks per buffer,
	// used to compute the audio buffer len: tpb * blocksize (always 64)
	int ticksPerBuffer = 8;	// 8 * 64 = buffer len of 512
	
	// setup the app core
	core.setup(2, 1, 44100, ticksPerBuffer);

	// setup OF sound stream
	ofSoundStreamSetup(2, 1, this, 44100, ofxPd::blockSize()*ticksPerBuffer, 3);
}

//--------------------------------------------------------------
void testApp::update() {
	core.update();
}

//--------------------------------------------------------------
void testApp::draw() {
	core.draw();
}

//--------------------------------------------------------------
void testApp::exit() {
	core.exit();
}

//--------------------------------------------------------------
void testApp::touchDown(ofTouchEventArgs &touch) {
	core.playTone(60);
}

//--------------------------------------------------------------
void testApp::touchMoved(ofTouchEventArgs &touch) {

}

//--------------------------------------------------------------
void testApp::touchUp(ofTouchEventArgs &touch) {

}

//--------------------------------------------------------------
void testApp::touchDoubleTap(ofTouchEventArgs &touch) {

}

//--------------------------------------------------------------
void testApp::lostFocus() {

}

//--------------------------------------------------------------
void testApp::gotFocus() {

}

//--------------------------------------------------------------
void testApp::gotMemoryWarning() {

}

//--------------------------------------------------------------
void testApp::deviceOrientationChanged(int newOrientation) {

}

//--------------------------------------------------------------
void testApp::touchCancelled(ofTouchEventArgs& args) {

}

//--------------------------------------------------------------
void testApp::audioReceived(float * input, int bufferSize, int nChannels) {
	core.audioReceived(input, bufferSize, nChannels);
}

void testApp::audioRequested(float * output, int bufferSize, int nChannels) {
	core.audioRequested(output, bufferSize, nChannels);
}
