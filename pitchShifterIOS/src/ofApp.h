/*
 * Copyright (c) 2012 Dan Wilcox <danomatika@gmail.com>
 * for Golan Levin & the CMU Studio for Creative Inquiry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See https://github.com/danomatika/ofxPd/examplePitchShifter for documentation
 *
 */
#pragma once

#include "ofMain.h"
#include "ofxiOS.h"
#include "ofxiOSExtras.h"

#include "ofxPd.h"
#include "ofxSimpleSlider.h"

// a namespace for the Pd types
using namespace pd;

// this example app is designed for an iPad in landscape mode,
// in the project settings Deployment Info set the following:
//   * Devices: iPad
//   * Device Orientation: disable Portrait and Upside Down
class ofApp : public ofxiOSApp, public PdReceiver {

	public:

		// main
		void setup();
		void update();
		void draw();
	
		void touchDown(ofTouchEventArgs &touch);
		void touchMoved(ofTouchEventArgs &touch);
		void touchUp(ofTouchEventArgs &touch);
		void touchDoubleTap(ofTouchEventArgs &touch);
		void touchCancelled(ofTouchEventArgs &touch);

		void lostFocus();
		void gotFocus();
		void gotMemoryWarning();
		void deviceOrientationChanged(int newOrientation);
		
		// audio callbacks
		void audioReceived(float * input, int bufferSize, int nChannels);
		void audioRequested(float * output, int bufferSize, int nChannels);
		
		// pd message receiver callbacks
		void print(const std::string& message);
	
		void receiveBang(const std::string& dest);
		void receiveFloat(const std::string& dest, float value);
		void receiveSymbol(const std::string& dest, const std::string& symbol);
		void receiveList(const std::string& dest, const List& list);
		void receiveMessage(const std::string& dest, const std::string& msg, const List& list);
		
		ofxPd pd; //< pd instance
		vector<float> scopeArray;

		// gui
		ofxSimpleSlider mixSlider, transposeSlider, inGainSlider, outGainSlider;
};
