#include "testApp.h"


//--------------------------------------------------------------
void testApp::setup()
{
	ofSetFrameRate(30);
	
	ofSetDataPathRoot("./data");
	char the_path[256];
    getcwd(the_path, 255);
	cout << "cwd: " << the_path << endl;
	
	// so far this is following the samples/test.c example from libpd
	
	pdInit();
	pdDspOn();
	pdOpenPatch("test.pd", "data");
	
	for(int i = 0; i < 10 * srate / 64; i++)
	{
    	pdUpdate();
  	}
}

//--------------------------------------------------------------
void testApp::update()
{
	ofBackground(100, 100, 100);
	
	//pdUpdate();
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
{
	libpd_process_float(inbuf, outbuf);
}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels)
{

}

//--------------------------------------------------------------
void testApp::pdPrintReceived(string message)
{
	cout << "print: " << message << endl;
}

void testApp::pdNoteonReceived(int channel, int pitch, int velocity)
{
	cout << "noteon: " << channel << " " << pitch << " " << velocity << endl;
}
