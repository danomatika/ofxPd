/*
 * Copyright (c) 2015 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxPd for documentation
 *
 */
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	//ofSetLogLevel("Pd", OF_LOG_VERBOSE); // see verbose info inside

	// the number of libpd ticks per buffer,
	// used to compute the audio buffer len: tpb * blocksize (always 64)
	#ifdef TARGET_LINUX_ARM
		// longer latency for Raspberry PI
		int ticksPerBuffer = 32; // 32 * 64 = buffer len of 2048
		int numInputs = 0; // no built in mic
	#else
		int ticksPerBuffer = 8; // 8 * 64 = buffer len of 512
		int numInputs = 1;
	#endif
	int numOutputs = 2;

	// allocate instance output buffers
	outputBufferSize = numOutputs*ticksPerBuffer*ofxPd::blockSize();
	outputBuffer1 = new float[outputBufferSize];
	outputBuffer2 = new float[outputBufferSize];
	memset(outputBuffer1, 0, outputBufferSize);
	memset(outputBuffer2, 0, outputBufferSize);

	// setup OF sound stream
	ofSoundStreamSettings settings;
	settings.numInputChannels = numInputs;
	settings.numOutputChannels = numOutputs;
	settings.sampleRate = 44100;
	settings.bufferSize = ofxPd::blockSize() * ticksPerBuffer;
	settings.setInListener(this);
	settings.setOutListener(this);
	ofSoundStreamSetup(settings);

	// allocate pd instance handles
	ofLog() << ofxPd::numInstances() - 1 << " instance(s)"; // - main instance
	ofLog() << "instance 1: " << ofToHex(pd1.instancePtr());
	ofLog() << "instance 2: " << ofToHex(pd2.instancePtr());

	// setup Pd
	//
	// set 4th arg to true for queued message passing using an internal ringbuffer,
	// this is useful if you need to control where and when the message callbacks
	// happen (ie. within a GUI thread)
	//
	// note: you won't see any message prints until update() is called since
	// the queued messages are processed there, this is normal
	if(!pd1.init(numOutputs, numInputs, 44100, ticksPerBuffer, false)) {
		ofExit(1);
	}
	pd1.setReceiver(this);

	// audio processing on
	pd1.start();

	// open patch
	pd1.openPatch("test.pd");

	// start the osc~ via sending [; 1003-frequency 440(
	// 1003 refers to the first $0 which is used within the test patch for the
	// receiver name: [r $0-1003]
	pd1 << StartMessage() << 440.0f << FinishMessage("1003-frequency", "float");

	// now do the same for instance 2
	if(!pd2.init(numOutputs, numInputs, 44100, ticksPerBuffer, false)) {
		ofExit(1);
	}
	pd2.setReceiver(this);
	pd2.start();
	pd2.openPatch("test.pd");

	// start the osc~ via sending [; 1000-frequency 880(
	pd2 << StartMessage() << 880.0f << FinishMessage("1003-frequency", "float");

	// check if we are really using multiple instances
	if(pd1.instancePtr() == pd2.instancePtr()) {
		ofLogError() << "Both instances are the same.";
		ofLogError() << "Is this example compiled with PDINSTANCE and PDTHREADS set?";
		ofExit();
	}
}

//--------------------------------------------------------------
void ofApp::update() {
	ofBackground(100, 100, 100);
	
	// since this is a test and we don't know if init() was called with
	// queued = true or not, we check it here
	if(pd1.isQueued()) {
		// process any received messages, if you're using the queue
		pd1.receiveMessages();
	}
	if(pd2.isQueued()) {
		pd2.receiveMessages();
	}
}

//--------------------------------------------------------------
void ofApp::draw() {}

//--------------------------------------------------------------
void ofApp::exit() {

	// cleanup
	ofSoundStreamStop();

	pd1.clear();
	pd2.clear();
}

//--------------------------------------------------------------
void ofApp::audioReceived(float * input, int bufferSize, int nChannels) {
	
	// process audio input for instance 1
	pd1.audioIn(input, bufferSize, nChannels);

	// process audio input for instance 2
	pd2.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void ofApp::audioRequested(float * output, int bufferSize, int nChannels) {

	// process audio output for instance 1
	pd1.audioOut(outputBuffer1, bufferSize, nChannels);
	
	// process audio output for instance 2
	pd2.audioOut(outputBuffer2, bufferSize, nChannels);

	// mix the two instance output buffers together
	for(int i = 0; i < outputBufferSize; i += 1) {
		output[i] = (outputBuffer1[i] + outputBuffer2[i]) * 0.5f; // simple mix
	}
}

//--------------------------------------------------------------
void ofApp::print(const std::string &message) {
	ofLog() << message;
}
