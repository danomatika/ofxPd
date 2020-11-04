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
	pdinstance1 = libpd_new_instance();
	pdinstance2 = libpd_new_instance();
	
	// set a "current" instance before pd.init() or else Pd will make
    // an unnecessary third "default" instance
	libpd_set_instance(pdinstance1);
	
	// setup Pd
	//
	// set 4th arg to true for queued message passing using an internal ringbuffer,
	// this is useful if you need to control where and when the message callbacks
	// happen (ie. within a GUI thread)
	//
	// note: you won't see any message prints until update() is called since
	// the queued messages are processed there, this is normal
	//
	// ... here we'd sure like to be able to have number of channels be
    // per-instance.  The sample rate is still global within Pd but we might
    // also consider relaxing that restrction.
	//
	if(!pd.init(numOutputs, numInputs, 44100, ticksPerBuffer, false)) {
		ofExit(1);
	}
	pd.setReceiver(this);
	
	// allocate instance output buffers
	outputBufferSize = numOutputs*ticksPerBuffer*ofxPd::blockSize();
	outputBuffer1 = new float[outputBufferSize];
	outputBuffer2 = new float[outputBufferSize];
	memset(outputBuffer1, 0, outputBufferSize);
	memset(outputBuffer2, 0, outputBufferSize);

	libpd_set_instance(pdinstance1);  // talk to first pd instance

	// audio processing on
	pd.start();

	// open patch
	pd.openPatch("test.pd");

	libpd_set_instance(pdinstance2); // talk to the second pd instance

	// audio processing on
	pd.start();

	// open patch
	pd.openPatch("test.pd");
	
	// The following two messages can be sent without setting the pd instance
	// and anyhow the symbols are global so they may affect multiple instances.
	// However, if the messages change anything in the pd instance structure
	// (DSP state; current time; list of all canvases n our instance) those
	// changes will apply to the current Pd nstance, so the earlier messages,
	// for instance, were sensitive to which was the current one.
	//
	// Note also that I'm using the fact that $0 is set to 1003, 1004, ...
	// as patches are opened, it would be better to open the patches with
	// settable $1, etc parameters to openPatch().
	
	// [; pd frequency 220 (
	pd << StartMessage() << 440.0f << FinishMessage("1003-frequency", "float");

	// [; pd frequency 440 (
	pd << StartMessage() << 880.0f << FinishMessage("1004-frequency", "float");
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
	
//	// run for 1 second and exit
//	if(ofGetElapsedTimef() > 1.0) {
//		ofExit(); // exit app
//	}
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
void ofApp::print(const std::string& message) {
	cout << message << endl;
}
