#include "AppCore.h"

#include <Poco/Path.h>

//--------------------------------------------------------------
void AppCore::setup(const int numOutChannels, const int numInChannels,
				    const int sampleRate, const int ticksPerBuffer) {

	ofSetFrameRate(30);
	//ofSetLogLevel(OF_LOG_VERBOSE);
	
	cout << Poco::Path::current() << endl;
	
	if(!pd.init(numOutChannels, numInChannels, sampleRate, ticksPerBuffer)) {
		ofLog(OF_LOG_ERROR, "Could not init pd");
		OF_EXIT_APP(1);
	}
	
	// add recieve source names
	pd.addSource("toOF");
	pd.addSource("env");
	
	// add listener
	pd.addListener(*this);
	pd.subscribe(*this);			// listen to everything
	pd.unsubscribe(*this, "env");	// don't listen to "env"
	
	//pd.subscribe(*this, "toOF");	// listen to "toOF"
	//pd.unsubscribe(*this);		// don't listen to anything
	
	pd.dspOn();
	
	// open patch
	Patch patch = pd.openPatch("test.pd");
	cout << patch << endl;
	
	// close patch
	pd.closePatch(patch);
	cout << patch << endl;
	
	// open patch
	patch = pd.openPatch("test.pd");
	cout << patch << endl;
	
	// test basic atoms
	pd.sendBang("fromOF");
	pd.sendFloat("fromOF", 100);
	pd.sendSymbol("fromOF", "test string");
	
	// play a tone by sending a list
	// [list tone pitch 72 (
	pd.startList("tone");
		pd.addSymbol("pitch");
		pd.addFloat(72);
	pd.finish();
	pd.sendBang("tone");
	
	// array check length
	cout << "array1 len: " << pd.getArrayLen("array1") << endl;
	
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
}

//--------------------------------------------------------------
void AppCore::update() {
	ofBackground(100, 100, 100);
}

//--------------------------------------------------------------
void AppCore::draw() {}

//--------------------------------------------------------------
void AppCore::exit() {}

//--------------------------------------------------------------
void AppCore::playTone(int pitch) {
	pd << StartList("tone") << "pitch" << pitch << Finish() << Bang("tone");
}

//--------------------------------------------------------------
void AppCore::keyPressed (int key) {

	switch(key) {
	
		case 'a':
			playTone(60);
			break;
			
		case 's':
			playTone(61);
			break;
			
		case 'd':
			playTone(62);
			break;
			
		case 'e':
			if(pd.isSubscribed(*this, "env")) {
				pd.unsubscribe(*this, "env");
			}
			else {
				pd.subscribe(*this, "env");
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
void AppCore::printReceived(const std::string& message) {
	cout << "print: " << message << endl;
}
		
void AppCore::bangReceived(const std::string& dest) {
	cout << "bang " << dest << endl;
}

void AppCore::floatReceived(const std::string& dest, float value) {
	cout << "float " << dest << ": " << value << endl;
}

void AppCore::symbolReceived(const std::string& dest, const std::string& symbol) {
	cout << "symbol " << dest << ": " << symbol << endl;
}

void AppCore::listReceived(const std::string& dest, const List& list) {
	cout << "list " << dest << ": " << list.toString() << endl;
	cout << "\t " << list.types() << endl;
}

void AppCore::messageReceived(const std::string& dest, const std::string& msg, const List& list) {
	cout << "message " << dest << ": " << msg << " " << list.toString() << endl;
	cout << "\t " << list.types() << endl;
}

void AppCore::noteReceived(const int channel, const int pitch, const int velocity) {
	cout << "note: " << channel << " " << pitch << " " << velocity << endl;
}
