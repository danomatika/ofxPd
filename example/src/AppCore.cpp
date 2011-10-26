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
#include "AppCore.h"

#include <Poco/Path.h>

//--------------------------------------------------------------
void AppCore::setup(const int numOutChannels, const int numInChannels,
				    const int sampleRate, const int ticksPerBuffer) {

	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	//ofSetLogLevel(OF_LOG_VERBOSE);
	
	cout << Poco::Path::current() << endl;
	
	if(!pd.init(numOutChannels, numInChannels, sampleRate, ticksPerBuffer)) {
		ofLog(OF_LOG_ERROR, "Could not init pd");
		OF_EXIT_APP(1);
	}
    
    midiChan = 1; // midi channels are 1-16
	
	// subscribe to receive source names
	pd.subscribe("toOF");
	pd.subscribe("env");
	
	// add message receiver
	pd.addReceiver(*this);   // automatically receives from all subscribed sources
	pd.ignore(*this, "env"); // don't receive from "env"
    //pd.ignore(*this);             // ignore all sources
	//pd.receive(*this, "toOF");	// receive only from "toOF"
	
    // add midi receiver
    pd.addMidiReceiver(*this);  // automatically receives from all channels
    //pd.ignoreMidi(*this, 1);     // ignore midi channel 1
    //pd.ignoreMidi(*this);         // ignore all channels
    //pd.receiveMidi(*this, 1);    // receive only from channel 1
    
	// add the data/pd folder to the search path
	pd.addToSearchPath("pd");
	
	// audio processing on
	pd.start();
	
	
	cout << endl << "BEGIN Patch Test" << endl;
	
	// open patch
	Patch patch = pd.openPatch("test.pd");
	cout << patch << endl;
	
	// close patch
	pd.closePatch(patch);
	cout << patch << endl;
	
	// open patch
	patch = pd.openPatch("test.pd");
	cout << patch << endl;
	
	cout << "FINISH Patch Test" << endl;
	
	
	
	cout << endl << "BEGIN Message Test" << endl;
	
	// test basic atoms
	pd.sendBang("fromOF");
	pd.sendFloat("fromOF", 100);
	pd.sendSymbol("fromOF", "test string");
	
	// send a list
	pd.startMsg();
		pd.addFloat(1.23);
		pd.addSymbol("a symbol");
	pd.finishList("fromOF");
	
	// send a message to the $0 reciever ie $0-toOF
	pd.startMsg();
		pd.addFloat(1.23);
		pd.addSymbol("a symbol");
	pd.finishList(patch.dollarZeroStr()+"-fromOF");
	
    // send a list using the List object
    List testList;
    testList.addFloat(1.23);
    testList.addSymbol("sent from a List object");
    pd.sendList("fromOF", testList);
    pd.sendMsg("fromOF", "msg", testList);
    
	cout << "FINISH Message Test" << endl;
	
	
	cout << endl << "BEGIN MIDI Test" << endl;
	
	// send functions
	pd.sendNote(midiChan, 60);
	pd.sendCtl(midiChan, 0, 64);
	pd.sendPgm(midiChan, 100);   // note: pgm num range is 1 - 128
	pd.sendBend(midiChan, 2000); // note: ofxPd uses -8192 - 8192 while [bendin] returns 0 - 16383,
                                 // so sending a val of 2000 gives 10192 in pd
	pd.sendTouch(midiChan, 100);
	pd.sendPolyTouch(midiChan, 64, 100);
	pd.sendMidiByte(0, 239);    // note: pd adds +2 to the port number from [midiin], [sysexin], & [realtimein]
	pd.sendSysExByte(0, 239);   // so sending to port 0 gives port 2 in pd
	pd.sendSysRtByte(0, 239);
	
	// stream
	pd << Note(midiChan, 60) << Ctl(midiChan, 100, 64) << Pgm(midiChan, 100)
       << Bend(midiChan, 2000) << Touch(midiChan, 100) << PolyTouch(midiChan, 64, 100)
	   << StartMidi(0) << 239 << Finish()
	   << StartSysEx(0) << 239 << Finish()
	   << StartSysRt(0) << 239 << Finish();
    
	cout << "FINISH MIDI Test" << endl;
	
	
	cout << endl << "BEGIN Array Test" << endl;
	
	// array check length
	cout << "array1 len: " << pd.getArraySize("array1") << endl;
	
	// read array
	std::vector<float> array1;
	pd.readArray("array1", array1);	// sets array to correct size
	cout << "array1 ";
	for(int i = 0; i < array1.size(); ++i)
		cout << array1[i] << " ";
	cout << endl;
	
	// write array
	for(int i = 0; i < array1.size(); ++i)
		array1[i] = i;
	pd.writeArray("array1", array1);
	
	// ready array
	pd.readArray("array1", array1);
	cout << "array1 ";
	for(int i = 0; i < array1.size(); ++i)
		cout << array1[i] << " ";
	cout << endl;
	
	// clear array
	pd.clearArray("array1", 10);
	
	// ready array
	pd.readArray("array1", array1);
	cout << "array1 ";
	for(int i = 0; i < array1.size(); ++i)
		cout << array1[i] << " ";
	cout << endl;

	cout << "FINISH Array Test" << endl;


	
	cout << endl << "BEGIN PD Test" << endl;
	pd.sendSymbol("fromOF", "test");
	cout << "FINISH PD Test" << endl << endl;
	
	
	
	// play a tone by sending a list
	// [list tone pitch 72 (
	pd.startMsg();
		pd.addSymbol("pitch");
		pd.addFloat(72);
	pd.finishList("tone");
	pd.sendBang("tone");
}

//--------------------------------------------------------------
void AppCore::update() {
	ofBackground(100, 100, 100);
	
	// update scope array from pd
	pd.readArray("scope", scopeArray);
}

//--------------------------------------------------------------
void AppCore::draw() {

	// draw scope
	ofSetColor(0, 255, 0);
	ofSetRectMode(OF_RECTMODE_CENTER);
	float x = 0, y = ofGetHeight()/2;
	float w = ofGetWidth() / (float) scopeArray.size(), h = ofGetHeight()/2;
	for(int i = 0; i < scopeArray.size()-1; ++i) {
		ofLine(x, y+scopeArray[i]*h, x+w, y+scopeArray[i+1]*h);
		x += w;
	}
}

//--------------------------------------------------------------
void AppCore::exit() {}

//--------------------------------------------------------------
void AppCore::playTone(int pitch) {
	pd << StartMsg() << "pitch" << pitch << FinishList("tone") << Bang("tone");
}

//--------------------------------------------------------------
void AppCore::keyPressed (int key) {

	switch(key) {
	
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
			if(pd.isReceiving(*this, "env")) {
				pd.ignore(*this, "env");
                cout << "ignoring env" << endl;
			}
			else {
				pd.receive(*this, "env");
                cout << "receiving from env" << endl;
			}
			break;
			
		default:
			break;
	}
}

//--------------------------------------------------------------
void AppCore::audioReceived(float * input, int bufferSize, int nChannels) {
	pd.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void AppCore::audioRequested(float * output, int bufferSize, int nChannels) {
	pd.audioOut(output, bufferSize, nChannels);
}

//--------------------------------------------------------------
void AppCore::receivePrint(const std::string& message) {
	cout << message << endl;
}

//--------------------------------------------------------------		
void AppCore::receiveBang(const std::string& dest) {
	cout << "OF: bang " << dest << endl;
}

void AppCore::receiveFloat(const std::string& dest, float value) {
	cout << "OF: float " << dest << ": " << value << endl;
}

void AppCore::receiveSymbol(const std::string& dest, const std::string& symbol) {
	cout << "OF: symbol " << dest << ": " << symbol << endl;
}

void AppCore::receiveList(const std::string& dest, const List& list) {
	cout << "OF: list " << dest << ": ";
	
	// step through the list
	for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			cout << list.asFloat(i) << " ";
		else if(list.isSymbol(i))
			cout << list.asSymbol(i) << " ";
	}
    
    // you can also use the built in toString function or simply stream it out
    // cout << list.toString();
    // cout << list;
	
    // print an OSC-style type string
	cout << list.types() << endl;
}

void AppCore::receiveMessage(const std::string& dest, const std::string& msg, const List& list) {
	cout << "OF: msg " << dest << ": " << msg << " " << list.toString() << list.types() << endl;
}

//--------------------------------------------------------------
void AppCore::receiveNote(const int channel, const int pitch, const int velocity) {
	cout << "OF MIDI: note: " << channel << " " << pitch << " " << velocity << endl;
}

void AppCore::receiveCtl(const int channel, const int controller, const int value) {
	cout << "OF MIDI: ctl: " << channel << " " << controller << " " << value << endl;
}

// note: pgm nums are 1-128 to match pd
void AppCore::receivePgm(const int channel, const int value) {
	cout << "OF MIDI: pgm: " << channel << " " << value << endl;
}

void AppCore::receiveBend(const int channel, const int value) {
	cout << "OF MIDI: bend: " << channel << " " << value << endl;
}

void AppCore::receiveTouch(const int channel, const int value) {
	cout << "OF MIDI: touch: " << channel << " " << value << endl;
}

void AppCore::receivePolyTouch(const int channel, const int pitch, const int value) {
	cout << "OF MIDI: polytouch: " << channel << " " << pitch << " " << value << endl;
}

// note: pd adds +2 to the port num, so sending to port 3 in pd to [midiout],
//       shows up at port 1 in ofxPd
void AppCore::receiveMidiByte(const int port, const int byte) {
	cout << "OF MIDI: midibyte: " << port << " " << byte << endl;
}
