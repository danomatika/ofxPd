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
#pragma once

#include "ofMain.h"

#include "ofxPd.h"

// a namespace for the Pd types
using namespace pd;

// This example demonstrates how to use the libpd multiple instance support with
// the ofxPd C++ wrapper. When compiled with -DPDINSTANCE and -DPDTHREADS set in
// *both* CFLAGS and CXXFLAGS, each ofxPd instance is separate with it's own
// internal state and message receivers. If the defines are not set, each ofxPd
// instance refers to the same libpd main instance.
//
// This example is adapted from the libpd pdtest_multi C example which is
// originally by Miller Puckette.
//
// *Do not* use this as your first approach to parallelize CPU hogging patches.
// It's highly suggested that you attempt to streamline your patches first before
// using this multiple instance support.
//
class ofApp : public ofBaseApp, public PdReceiver {

	public:

		// main
		void setup();
		void update();
		void draw();
		void exit();
		
		// audio callbacks
		void audioReceived(float * input, int bufferSize, int nChannels);
		void audioRequested(float * output, int bufferSize, int nChannels);
		
		// pd message receiver callbacks
		void print(const std::string &message);
	
		// pd instances
		ofxPd pd1, pd2;
	
		int outputBufferSize; //< audio output buffer size
		float *outputBuffer1; //< interleaved audio output buffer for instance 1
		float *outputBuffer2; //< interleaved audio output buffer for instance 2
};
