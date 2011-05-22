#include "testApp.h"


#include <Poco/Path.h>

//--------------------------------------------------------------
void testApp::setup() {

	ofSetFrameRate(30);
	//ofSetLogLevel(OF_LOG_VERBOSE);
	
	cout << Poco::Path::current() << endl;
	
	// setup OF sound stream
	ofSoundStreamSetup(2, 2, this, 44100, ofxPd::getBlockSize(), 4);
	
	pd.init(2, 2, 44100);
	
	pd.addSource("toOF");
	pd.addSource("env");
	pd.addListener(*this);
	pd.subscribe(*this);			// listen to everything
	pd.unsubscribe(*this, "env");	// don't listen to "env"
	
	//pd.subscribe(*this, "toOF");	// listen to "toOF"
	//pd.unsubscribe(*this);		// don't listen to anything
	
	pd.dspOn();
	pd.openPatch("test.pd");
	
	pd.sendBang("fromOF");
	pd.sendFloat("fromOF", 100);
	pd.sendSymbol("fromOF", "test string");
	
	
	pd.startList("tone");
		pd.addSymbol("pitch");
		pd.addFloat(72);
	pd.finish();
	pd.sendBang("tone");
}

//--------------------------------------------------------------
void testApp::update() {
	ofBackground(100, 100, 100);
}

//--------------------------------------------------------------
void testApp::draw() {}

//--------------------------------------------------------------
void testApp::exit() {}

//--------------------------------------------------------------
void testApp::keyPressed (int key) {

	switch(key) {
	
		case 'a':
			pd << StartList("tone") << "pitch" << 60 << Finish() << Bang("tone");
			break;
			
		case 's':
			pd << StartList("tone") << "pitch" << 61 << Finish() << Bang("tone");
			break;
			
		case 'd':
			pd << StartList("tone") << "pitch" << 62 << Finish() << Bang("tone");
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
void testApp::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button) {}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button) {}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button) {}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h) {}

//--------------------------------------------------------------
void testApp::audioReceived(float * input, int bufferSize, int nChannels) {
	pd.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels) {
	pd.audioOut(output, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::printReceived(const std::string& message) {
	cout << "print: " << message << endl;
}
		
void testApp::bangReceived(const std::string& dest) {
	cout << "bang " << dest << endl;
}

void testApp::floatReceived(const std::string& dest, float value) {
	cout << "float " << dest << ": " << value << endl;
}

void testApp::symbolReceived(const std::string& dest, const std::string& symbol) {
	cout << "symbol " << dest << ": " << symbol << endl;
}

void testApp::listReceived(const std::string& dest, const List& list) {
	cout << "list " << dest << ": " << endl;
}

void testApp::messageReceived(const std::string& dest, const std::string& msg, const List& list) {
	cout << "message " << dest << ": " << msg << endl;
}

void testApp::noteReceived(const int channel, const int pitch, const int velocity) {
	cout << "note: " << channel << " " << pitch << " " << velocity << endl;
}
