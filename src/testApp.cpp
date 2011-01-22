#include "testApp.h"


#include <Poco/Path.h>

//--------------------------------------------------------------
void testApp::setup()
{
	ofSetFrameRate(30);
	
	cout << Poco::Path::current() << endl;
	ofLogSetTopicLogLevel("ofxPd", OF_LOG_VERBOSE);
	
	// setup OF sound stream
	ofSoundStreamSetup(0, 2, this, 44100, 64, 4);
	left = new float[256];
	right = new float[256];
	
	pdInit();
	pdDspOn();
	pdOpenPatch("test.pd");
	pdBind("toOF");
	pdSendBang("fromOF");
	pdSendFloat("fromOF", 100);
	
	for(int i = 0; i < 10 * 44100 / 64; ++i)
		pdUpdate();
}

//--------------------------------------------------------------
void testApp::update()
{
	ofBackground(100, 100, 100);
}

//--------------------------------------------------------------
void testApp::draw()
{
}

//--------------------------------------------------------------
void testApp::exit()
{}

//--------------------------------------------------------------
void testApp::keyPressed (int key)
{}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y)
{}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h)
{}

//--------------------------------------------------------------
void testApp::audioReceived(float * input, int bufferSize, int nChannels)
{}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels)
{}

//--------------------------------------------------------------
void testApp::pdPrintReceived(string message)
{
	//ofLogNotice() << "print: " << message;
}

void testApp::pdNoteonReceived(int channel, int pitch, int velocity)
{
	//ofLogNotice() << "noteon: " << channel << " " << pitch << " " << velocity;
}
