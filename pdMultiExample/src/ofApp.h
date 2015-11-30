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
#include "z_libpd.h" // currently needed for pd types
#include "m_imp.h" // currently needed for t_pdinstance type

// a namespace for the Pd types
using namespace pd;

// This example demonstrates how to use the currently *experimental* libpd
// multiple instance support with the ofxPd C++ wrapper. You essentially
// create t_pdinstance types which act as instance "handles" (aka pointer or ID)
// and then tell pd which instance you currently want all libpd/ofxPd commands to
// target using the pd_setinstance() function.
//
// This example is adapted from the libpd pdtest_multi example which is originally
// by Miller Puckette.
//
// Note: This multiple instance support is currently EXPERIMENTAL. Use at your own
// risk. This API is subject to change.
//
// *Do not* use this as your first approach to parallelize CPU hogging patches.
// It's highly suggested that you attempt to streamline your patches first before
// using this multi instance support.
//
class ofApp : public ofBaseApp, public PdReceiver{

	public:

		// main
		void setup();
		void update();
		void draw();
		
		// audio callbacks
		void audioReceived(float * input, int bufferSize, int nChannels);
		void audioRequested(float * output, int bufferSize, int nChannels);
		
		// pd message receiver callbacks
		void print(const std::string& message);
	
		// main ofxPd object we use to access libpd
		ofxPd pd;
	
		// pd instance handles, not full fledged ofxPd objects yet, but internal
		// pd types which tell libpd to address a separate internal "instance"
		t_pdinstance *pdinstance1, *pdinstance2;
	
		// used to avoid concurrency segfaults between main and audio thread
		// when accessing currently non thread safe pd instance functions
		ofMutex instanceMutex;
	
		int outputBufferSize; //< audio output buffer size
		float* outputBuffer1; //< interleaved audio output buffer for instance 1
		float* outputBuffer2; //< interleaved audio output buffer for instance 2
};
