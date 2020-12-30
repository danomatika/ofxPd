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

	// init before creating any instances
	libpd_init();

	// allocate pd instance handles
	pdinstance1 = libpd_new_instance();
	pdinstance2 = libpd_new_instance();
	ofLog() << libpd_num_instances() << " instances";
	ofLog() << "instance 1: " << ofToHex(pdinstance1);
	ofLog() << "instance 2: " << ofToHex(pdinstance2);

	// set an instance before pd.init() or else Pd will use the default instance
	libpd_set_instance(pdinstance1); // talk to first pd instance
	
	// setup Pd
	//
	// set 4th arg to true for queued message passing using an internal ringbuffer,
	// this is useful if you need to control where and when the message callbacks
	// happen (ie. within a GUI thread)
	//
	// note: you won't see any message prints until update() is called since
	// the queued messages are processed there, this is normal
	if(!pd.init(numOutputs, numInputs, 44100, ticksPerBuffer, false)) {
		ofExit(1);
	}
	pd.setReceiver(this);

	// audio processing on
	pd.start();

	// open patch
	pd.openPatch("test.pd");

	// start the osc~ via sending [; 1003-frequency 440(
	// 1003 refers to the first $0 which is used within the test patch for the
	// receiver name: [r $0-1003]
	pd << StartMessage() << 440.0f << FinishMessage("1003-frequency", "float");

	// now do the same for instance 2
	libpd_set_instance(pdinstance2); // talk to the second pd instance
	if(!pd.init(numOutputs, numInputs, 44100, ticksPerBuffer, false)) {
		ofExit(1);
	}
	pd.setReceiver(this);
	pd.start();
	pd.openPatch("test.pd");

	// start the osc~ via sending [; 1000-frequency 880(
	pd << StartMessage() << 880.0f << FinishMessage("1003-frequency", "float");

	// check if we are really using multiple instances
	if(!pdinstance1 || !pdinstance2) {
		ofLogError() << "One or both instances are NULL.";
		ofLogError() << "Is this example compiled with PDINSTANCE and PDTHREADS set?";
		ofExit();
	}
}

//--------------------------------------------------------------
void ofApp::update() {
	ofBackground(100, 100, 100);
	
	// since this is a test and we don't know if init() was called with
	// queued = true or not, we check it here
	if(pd.isQueued()) {
		// process any received messages, if you're using the queue
		pd.receiveMessages();
	}
}

//--------------------------------------------------------------
void ofApp::draw() {}

//--------------------------------------------------------------
void ofApp::exit() {

	// cleanup
	ofSoundStreamStop();

	libpd_free_instance(pdinstance1);
	libpd_free_instance(pdinstance2);
}

//--------------------------------------------------------------
void ofApp::audioReceived(float * input, int bufferSize, int nChannels) {
	
	// process audio input for instance 1
	libpd_set_instance(pdinstance1);
	pd.audioIn(input, bufferSize, nChannels);

	// process audio input for instance 2
	libpd_set_instance(pdinstance2);
	pd.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void ofApp::audioRequested(float * output, int bufferSize, int nChannels) {

	// process audio output for instance 1
	libpd_set_instance(pdinstance1);
	pd.audioOut(outputBuffer1, bufferSize, nChannels);
	
	// process audio output for instance 2
	libpd_set_instance(pdinstance2);
	pd.audioOut(outputBuffer2, bufferSize, nChannels);

	// mix the two instance output buffers together
	for(int i = 0; i < outputBufferSize; i += 1) {
		output[i] = (outputBuffer1[i] + outputBuffer2[i]) * 0.5f; // mix
	}
}

//--------------------------------------------------------------
void ofApp::print(const std::string &message) {
	ofLog() << message;
}
