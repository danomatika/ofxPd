#include "testApp.h"


#include <Poco/Path.h>

//--------------------------------------------------------------
void testApp::setup() {

	ofSetFrameRate(30);
	
	cout << Poco::Path::current() << endl;
	
	// setup OF sound stream
	ofSoundStreamSetup(2, 2, this, 44100, ofxPd::getBlockSize(), 4);
	
	pd.init(2, 2, 44100);
	pd.addListener(*this);
	pd.dspOn();
	pd.openPatch("test.pd");
	pd.addSource("toOF");
	pd.sendBang("fromOF");
	pd.sendFloat("fromOF", 100);
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
void testApp::keyPressed (int key) {}

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
void testApp::audioReceived(float * input, int bufferSize, int nChannels)
{
	pd.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels)
{
	pd.audioOut(output, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::pdPrintReceived(string message)
{
	//ofLogNotice() << "print: " << message;
}

void testApp::pdNoteonReceived(int channel, int pitch, int velocity)
{
	//ofLogNotice() << "noteon: " << channel << " " << pitch << " " << velocity;
}
