/*
 * Copyright (c) 2012 Dan Wilcox <danomatika@gmail.com>
 * for Golan Levin & the CMU Studio for Creative Inquiry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See https://github.com/danomatika/ofxPd/examplePitchShifter for documentation
 *
 */
#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup() {

	// register touch events
	ofRegisterTouchEvents(this);
	
	// initialize the accelerometer
	ofxAccelerometer.setup();
	
	// iOSAlerts will be sent to this
	ofxiOSAlerts.addListener(this);
	
	// if you want a landscape orientation 
	ofSetOrientation(OF_ORIENTATION_90_RIGHT);
	
	ofBackground(127, 127, 127);
	
	// the number if libpd ticks per buffer,
	// used to compute the audio buffer len: tpb * blocksize (always 64)
	int ticksPerBuffer = 2; // 8 * 64 = buffer len of 512
	
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
