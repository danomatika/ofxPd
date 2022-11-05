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
#include "ofApp.h"

#import <AVFoundation/AVFoundation.h>

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(127, 127, 127);
	//ofSetLogLevel("Pd", OF_LOG_VERBOSE); // see verbose info inside

	// double check where we are ...
	std::cout << ofFilePath::getCurrentWorkingDirectory() << std::endl;

	// register touch events
	ofRegisterTouchEvents(this);
	
	// iOSAlerts will be sent to this
	ofxiOSAlerts.addListener(this);
	
	// try to set the preferred iOS sample rate, but get the actual sample rate
	// being used by the AVSession since newer devices like the iPhone 6S only
	// want specific values (ie 48000 instead of 44100)
	float sampleRate = setAVSessionSampleRate(44100);
	
	// the number if libpd ticks per buffer,
	// used to compute the audio buffer len: tpb * blocksize (always 64)
	int ticksPerBuffer = 8; // 8 * 64 = buffer len of 512

	// setup OF sound stream using the current *actual* samplerate
	ofSoundStreamSettings settings;
	settings.numInputChannels = 1;
	settings.numOutputChannels = 2;
	settings.sampleRate = sampleRate;
	settings.bufferSize = ofxPd::blockSize() * ticksPerBuffer;
	settings.setInListener(this);
	settings.setOutListener(this);
	ofSoundStreamSetup(settings);

	// setup Pd
	//
	// set 4th arg to true for queued message passing using an internal ringbuffer,
	// this is useful if you need to control where and when the message callbacks
	// happen (ie. within a GUI thread)
	//
	// note: you won't see any message prints until update() is called since
	// the queued messages are processed there, this is normal
	//
	if(!pd.init(2, 1, sampleRate, ticksPerBuffer-1, false)) {
		OF_EXIT_APP(1);
	}

	midiChan = 1; // midi channels are 1-16

	// subscribe to receive source names
	pd.subscribe("toOF");
	pd.subscribe("env");

	// add message receiver, required if you want to receieve messages
	pd.addReceiver(*this);   // automatically receives from all subscribed sources
	pd.ignoreSource(*this, "env");      // don't receive from "env"
	//pd.ignoreSource(*this);           // ignore all sources
	//pd.receiveSource(*this, "toOF");  // receive only from "toOF"

	// add midi receiver, required if you want to recieve midi messages
	pd.addMidiReceiver(*this);  // automatically receives from all channels
	//pd.ignoreMidiChannel(*this, 1);     // ignore midi channel 1
	//pd.ignoreMidiChannel(*this);        // ignore all channels
	//pd.receiveMidiChannel(*this, 1);    // receive only from channel 1

	// add the data/pd folder to the search path
	pd.addToSearchPath("pd/abs");

	// audio processing on
	pd.start();

	// -----------------------------------------------------
	std::cout << std::endl << "BEGIN Patch Test" << std::endl;

	// open patch
	Patch patch = pd.openPatch("pd/test.pd");
	std::cout << patch << std::endl;

	// close patch
	pd.closePatch(patch);
	std::cout << patch << std::endl;

	// open patch again
	patch = pd.openPatch(patch);
	std::cout << patch << std::endl;

	std::cout << "FINISH Patch Test" << std::endl;

	// -----------------------------------------------------
	std::cout << std::endl << "BEGIN Message Test" << std::endl;

	// test basic atoms
	pd.sendBang("fromOF");
	pd.sendFloat("fromOF", 100);
	pd.sendSymbol("fromOF", "test string");

	// stream interface
	pd << Bang("fromOF")
	   << Float("fromOF", 100)
	   << Symbol("fromOF", "test string");

	// send a list
	pd.startMessage();
	pd.addFloat(1.23);
	pd.addSymbol("a symbol");
	pd.finishList("fromOF");

	// send a message to the $0 receiver ie $0-fromOF
	pd.startMessage();
	pd.addFloat(1.23);
	pd.addSymbol("a symbol");
	pd.finishList(patch.dollarZeroStr()+"-fromOF");

	// send a list using the List object
	pd::List testList;
	testList.addFloat(1.23);
	testList.addSymbol("sent from a List object");
	pd.sendList("fromOF", testList);
	pd.sendMessage("fromOF", "msg", testList);

	// stream interface for list
	pd << StartMessage() << 1.23 << "sent from a streamed list" << FinishList("fromOF");

	std::cout << "FINISH Message Test" << std::endl;

	// -----------------------------------------------------
	std::cout << std::endl << "BEGIN MIDI Test" << std::endl;

	// send functions
	pd.sendNoteOn(midiChan, 60);
	pd.sendControlChange(midiChan, 0, 64);
	pd.sendProgramChange(midiChan, 100);    // note: pgm num range is 1 - 128
	pd.sendPitchBend(midiChan, 2000);   // note: ofxPd uses -8192 - 8192 while [bendin] returns 0 - 16383,
										// so sending a val of 2000 gives 10192 in pd
	pd.sendAftertouch(midiChan, 100);
	pd.sendPolyAftertouch(midiChan, 64, 100);
	pd.sendMidiByte(0, 239);    // note: pd adds +2 to the port number from [midiin], [sysexin], & [realtimein]
	pd.sendSysex(0, 239);       // so sending to port 0 gives port 2 in pd
	pd.sendSysRealTime(0, 239);

	// stream
	pd << NoteOn(midiChan, 60) << ControlChange(midiChan, 100, 64)
	   << ProgramChange(midiChan, 100) << PitchBend(midiChan, 2000)
	   << Aftertouch(midiChan, 100) << PolyAftertouch(midiChan, 64, 100)
	   << StartMidi(0) << 239 << Finish()
	   << StartSysex(0) << 239 << Finish()
	   << StartSysRealTime(0) << 239 << Finish();

	std::cout << "FINISH MIDI Test" << std::endl;

	// -----------------------------------------------------
	std::cout << std::endl << "BEGIN Array Test" << std::endl;

	// array check length
	std::cout << "array1 len: " << pd.arraySize("array1") << std::endl;

	// read array
	std::vector<float> array1;
	pd.readArray("array1", array1);	// sets array to correct size
	std::cout << "array1 ";
	for(int i = 0; i < array1.size(); ++i) {
		std::cout << array1[i] << " ";
	}
	std::cout << std::endl;

	// write array
	for(int i = 0; i < array1.size(); ++i)
		array1[i] = i;
	pd.writeArray("array1", array1);

	// ready array
	pd.readArray("array1", array1);
	std::cout << "array1 ";
	for(int i = 0; i < array1.size(); ++i) {
		std::cout << array1[i] << " ";
	}
	std::cout << std::endl;

	// clear array
	pd.clearArray("array1", 10);

	// ready array
	pd.readArray("array1", array1);
	std::cout << "array1 ";
	for(int i = 0; i < array1.size(); ++i) {
		std::cout << array1[i] << " ";
	}
	std::cout << std::endl;

	std::cout << "FINISH Array Test" << std::endl;

	// -----------------------------------------------------
	std::cout << std::endl << "BEGIN PD Test" << std::endl;

	pd.sendSymbol("fromOF", "test");

	std::cout << "FINISH PD Test" << std::endl;

	// -----------------------------------------------------
	std::cout << std::endl << "BEGIN Instance Test" << std::endl;

	// open 10 instances
	for(int i = 0; i < 10; ++i) {
		Patch p = pd.openPatch("pd/instance.pd");
		instances.push_back(p);
	}

	// send a hello bang to each instance individually using the dollarZero
	// to [r $0-instance] which should print the instance dollarZero unique id
	// and a unique random number
	for(int i = 0; i < instances.size(); ++i) {
		pd.sendBang(instances[i].dollarZeroStr()+"-instance");
	}

	// send a random float between 0 and 100
	for(int i = 0; i < instances.size(); ++i) {
		pd.sendFloat(instances[i].dollarZeroStr()+"-instance", int(ofRandom(0, 100)));
	}

	// send a symbol
	for(int i = 0; i < instances.size(); ++i) {
		pd.sendSymbol(instances[i].dollarZeroStr()+"-instance", "howdy dude");
	}

	// close all instances
	for(int i = 0; i < instances.size(); ++i) {
		pd.closePatch(instances[i]);
	}
	instances.clear();

	std::cout << "FINISH Instance Test" << std::endl;

	// -----------------------------------------------------
	// play a tone by sending a list
	// [list tone pitch 72 (
	pd.startMessage();
	pd.addSymbol("pitch");
	pd.addFloat(72);
	pd.finishList("tone");
	pd.sendBang("tone");

}

//--------------------------------------------------------------
void ofApp::update() {
	ofBackground(100, 100, 100);
	
	// since this is a test and we don't know if init() was called with
	// queued = true or not, we check it here
	if(pd.isQueued()) {
		// process any received messages, if you're using the queue and *do not*
		// call these, you won't receieve any messages or midi!
		pd.receiveMessages();
		pd.receiveMidi();
	}

	// update scope array from pd
	pd.readArray("scope", scopeArray);
}

//--------------------------------------------------------------
void ofApp::draw() {

	// draw scope
	ofSetColor(0, 255, 0);
	ofSetRectMode(OF_RECTMODE_CENTER);
	float x = 0, y = ofGetHeight()/2;
	float w = ofGetWidth() / (float) scopeArray.size(), h = ofGetHeight()/2;
	for(int i = 0; i < scopeArray.size()-1; ++i) {
		ofDrawLine(x, y+scopeArray[i]*h, x+w, y+scopeArray[i+1]*h);
		x += w;
	}
}

//--------------------------------------------------------------
void ofApp::exit() {

	// cleanup
	ofSoundStreamStop();
}

//--------------------------------------------------------------
void ofApp::keyPressed (int key) {

	switch(key) {

		// musical keyboard if you have a usb keyboard
		case 'a':
			playTone(60);
			break;
		case 'w':
			playTone(61);
			break;
		case 's':
			playTone(62);
			break;
		case 'e':
			playTone(63);
			break;
		case 'd':
			playTone(64);
			break;
		case 'f':
			playTone(65);
			break;
		case 't':
			playTone(66);
			break;
		case 'g':
			playTone(67);
			break;
		case 'y':
			playTone(68);
			break;
		case 'h':
			playTone(69);
			break;
		case 'u':
			playTone(70);
			break;
		case 'j':
			playTone(71);
			break;
		case 'k':
			playTone(72);
			break;

		case ' ':
			if(pd.isReceivingSource(*this, "env")) {
				pd.ignoreSource(*this, "env");
				std::cout << "ignoring env" << std::endl;
			}
			else {
				pd.receiveSource(*this, "env");
				std::cout << "receiving from env" << std::endl;
			}
			break;

		default:
			break;
	}
}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs &touch) {
	// y pos changes pitch
	int pitch = (-1 * (touch.y/ofGetHeight()) + 1) * 127;
	playTone(pitch);
}

//--------------------------------------------------------------
void ofApp::touchMoved(ofTouchEventArgs &touch) {}

//--------------------------------------------------------------
void ofApp::touchUp(ofTouchEventArgs &touch) {}

//--------------------------------------------------------------
void ofApp::touchDoubleTap(ofTouchEventArgs &touch) {}

//--------------------------------------------------------------
void ofApp::touchCancelled(ofTouchEventArgs &args) {}

//--------------------------------------------------------------
void ofApp::lostFocus() {}

//--------------------------------------------------------------
void ofApp::gotFocus() {}

//--------------------------------------------------------------
void ofApp::gotMemoryWarning() {}

//--------------------------------------------------------------
void ofApp::deviceOrientationChanged(int newOrientation) {}

//--------------------------------------------------------------
void ofApp::audioReceived(float * input, int bufferSize, int nChannels) {
	pd.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void ofApp::audioRequested(float * output, int bufferSize, int nChannels) {
	pd.audioOut(output, bufferSize, nChannels);
}

//--------------------------------------------------------------
void ofApp::print(const std::string &message) {
	std::cout << message << std::endl;
}

//--------------------------------------------------------------
void ofApp::receiveBang(const std::string &dest) {
	std::cout << "OF: bang " << dest << std::endl;
}

void ofApp::receiveFloat(const std::string &dest, float value) {
	std::cout << "OF: float " << dest << ": " << value << std::endl;
}

void ofApp::receiveSymbol(const std::string &dest, const std::string &symbol) {
	std::cout << "OF: symbol " << dest << ": " << symbol << std::endl;
}

void ofApp::receiveList(const std::string &dest, const pd::List &list) {
	std::cout << "OF: list " << dest << ": ";

	// step through the list
	for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			std::cout << list.getFloat(i) << " ";
		else if(list.isSymbol(i))
			std::cout << list.getSymbol(i) << " ";
	}

	// you can also use the built in toString function or simply stream it out
	// std::cout << list.toString();
	// std::cout << list;

	// print an OSC-style type string
	std::cout << list.types() << std::endl;
}

void ofApp::receiveMessage(const std::string &dest, const std::string &msg, const pd::List &list) {
	std::cout << "OF: message " << dest << ": " << msg << " " << list.toString() << list.types() << std::endl;
}

//--------------------------------------------------------------
void ofApp::receiveNoteOn(const int channel, const int pitch, const int velocity) {
	std::cout << "OF MIDI: note on: " << channel << " " << pitch << " " << velocity << std::endl;
}

void ofApp::receiveControlChange(const int channel, const int controller, const int value) {
	std::cout << "OF MIDI: control change: " << channel << " " << controller << " " << value << std::endl;
}

// note: pgm nums are 1-128 to match pd
void ofApp::receiveProgramChange(const int channel, const int value) {
	std::cout << "OF MIDI: program change: " << channel << " " << value << std::endl;
}

void ofApp::receivePitchBend(const int channel, const int value) {
	std::cout << "OF MIDI: pitch bend: " << channel << " " << value << std::endl;
}

void ofApp::receiveAftertouch(const int channel, const int value) {
	std::cout << "OF MIDI: aftertouch: " << channel << " " << value << std::endl;
}

void ofApp::receivePolyAftertouch(const int channel, const int pitch, const int value) {
	std::cout << "OF MIDI: poly aftertouch: " << channel << " " << pitch << " " << value << std::endl;
}

// note: pd adds +2 to the port num, so sending to port 3 in pd to [midiout],
//       shows up at port 1 in ofxPd
void ofApp::receiveMidiByte(const int port, const int byte) {
	std::cout << "OF MIDI: midi byte: " << port << " " << byte << std::endl;
}

//--------------------------------------------------------------
void ofApp::playTone(int pitch) {
	pd << StartMessage() << "pitch" << pitch << FinishList("tone") << Bang("tone");
}

//--------------------------------------------------------------
// set the samplerate the Apple approved way since newer devices
// like the iPhone 6S only allow certain sample rates,
// the following code may not be needed once this functionality is
// incorporated into the ofxiOSSoundStream
// thanks to Seth aka cerupcat
float ofApp::setAVSessionSampleRate(float preferredSampleRate) {
	
	NSError *audioSessionError = nil;
	AVAudioSession *session = [AVAudioSession sharedInstance];

	// disable active
	[session setActive:NO error:&audioSessionError];
	if (audioSessionError) {
		NSLog(@"Error %ld, %@", (long)audioSessionError.code, audioSessionError.localizedDescription);
	}

	// set category
	[session setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionAllowBluetooth|AVAudioSessionCategoryOptionMixWithOthers|AVAudioSessionCategoryOptionDefaultToSpeaker error:&audioSessionError];
	if(audioSessionError) {
		NSLog(@"Error %ld, %@", (long)audioSessionError.code, audioSessionError.localizedDescription);
	}

	// try to set the preferred sample rate
	[session setPreferredSampleRate:preferredSampleRate error:&audioSessionError];
	if(audioSessionError) {
		NSLog(@"Error %ld, %@", (long)audioSessionError.code, audioSessionError.localizedDescription);
	}

	// *** Activate the audio session before asking for the "current" values ***
	[session setActive:YES error:&audioSessionError];
	if (audioSessionError) {
		NSLog(@"Error %ld, %@", (long)audioSessionError.code, audioSessionError.localizedDescription);
	}
	ofLogNotice() << "AVSession samplerate: " << session.sampleRate << ", I/O buffer duration: " << session.IOBufferDuration;

	// our actual samplerate, might be differnt aka 48k on iPhone 6S
	return session.sampleRate;
}
