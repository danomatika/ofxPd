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
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	
	// the number of libpd ticks per buffer,
	// used to compute the audio buffer len: tpb * blocksize (always 64)
	#ifdef TARGET_LINUX_ARM
		// longer latency for Raspberry PI
		int ticksPerBuffer = 32; // 32 * 64 = buffer len of 2048
		
		// you'll need a USB mic on the Raspberry PI and may need to set the audio device,
		// otherwise this app won't really do anything without an incoming audio signal ...
		int numInputs = 0; // no built in mic, change this if you have a USB mic
	#else
		int ticksPerBuffer = 8; // 8 * 64 = buffer len of 512
		int numInputs = 1;
	#endif

	// setup OF sound stream
	ofSoundStreamSettings settings;
	settings.numInputChannels = numInputs;
	settings.numOutputChannels = 2;
	settings.sampleRate = 44100;
	settings.bufferSize = ofxPd::blockSize() * ticksPerBuffer;
	settings.setInListener(this);
	settings.setOutListener(this);
	ofSoundStreamSetup(settings);
	
	// setup pd
	if(!pd.init(2, numInputs, 44100, ticksPerBuffer)) {
		OF_EXIT_APP(1);
	}
	pd.subscribe("mix");
	pd.subscribe("transpose");
	pd.subscribe("inputGain");
	pd.subscribe("outputGain");
	pd.addToSearchPath("pd");
	pd.start();

	// open patch
	Patch patch = pd.openPatch("pd/_main.pd");
	std::cout << patch << std::endl;

	// setup GUI
	int x = -12, width = 100, step = 75;
	x += step;
	transposeSlider.setup(x, 34, width, 700, -12, 12, 0, true, true); x += width + step*2;
	mixSlider.setup(x, 34, width, 700, 0, 1, 1.0, true, true); x += width + step*2;
	inGainSlider.setup(x, 34, width, 700, 0, 1, 0.5, true, true); x += width + step*2;
	outGainSlider.setup(x, 34, 100, 700, 0, 1, 0.25, true, true);
	transposeSlider.setLabelString("Transpose");
	mixSlider.setLabelString("Wet/Dry Mix");
	inGainSlider.setLabelString("Input Gain");
	outGainSlider.setLabelString("Output Gain");
}

//--------------------------------------------------------------
void ofApp::update() {
	ofBackground(0, 0, 0);
	
	// update scope array from pd
	pd.readArray("scope", scopeArray);
	
	// udpate pd from gui
	pd << StartMessage() << "transpose" << transposeSlider.getValue() << FinishList("TO_PD");
	pd << StartMessage() << "mix" << mixSlider.getValue() << FinishList("TO_PD");
	pd << StartMessage() << "inGain" << inGainSlider.getValue() << FinishList("TO_PD");
	pd << StartMessage() << "outGain" << outGainSlider.getValue() << FinishList("TO_PD");
}

//--------------------------------------------------------------
void ofApp::draw() {

	// draw scope
	ofSetColor(0, 255, 0, 127);
	ofSetRectMode(OF_RECTMODE_CENTER);
	ofSetLineWidth(2.0);
	float x = 1, y = ofGetHeight()/2;
	float w = ofGetWidth() / (float) scopeArray.size(), h = ofGetHeight()/2;
	for(int i = 0; i < scopeArray.size()-1; ++i) {
		ofDrawLine(x, y+scopeArray[i]*h, x+w, y+scopeArray[i+1]*h);
		x += w;
	}
	ofSetLineWidth(1.0);
}

//--------------------------------------------------------------
void ofApp::exit() {

	// cleanup
	ofSoundStreamStop();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer &buffer) {
	pd.audioIn(buffer);
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer &buffer) {
	pd.audioOut(buffer);
}

//--------------------------------------------------------------
void ofApp::print(const std::string &message) {
	std::cout << message << std::endl;
}
