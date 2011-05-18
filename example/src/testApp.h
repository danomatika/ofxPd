#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"

#include "ofxPd.h"

class testApp : public ofBaseApp, public ofxPdListener {

	public:

		void setup();
		void update();
		void draw();
        void exit();

		void keyPressed  (int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		
		void audioReceived(float * input, int bufferSize, int nChannels);
		void audioRequested(float * output, int bufferSize, int nChannels);
		
		// pd callbacks
		void printReceived(const std::string& message);
		
		void bangReceived(const std::string& dest);
		void floatReceived(const std::string& dest, float value);
		void symbolReceived(const std::string& dest, const std::string& symbol);
		void listReceived(const std::string& dest, const List& list);
		void messageReceived(const std::string& dest, const std::string& msg, const List& list);
		
		void noteReceived(const int channel, const int pitch, const int velocity);
		
		ofxPd pd;
};

#endif
