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
#include "ofMain.h"
#include "ofApp.h"

// quick fix until 64bit OF on iOS,
// from https://github.com/openframeworks/openFrameworks/issues/3178
//#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR <= 8
//extern "C" {
//    size_t fwrite$UNIX2003(const void *a, size_t b, size_t c, FILE *d) {
//        return fwrite(a, b, c, d);
//    }
//    char* strerror$UNIX2003(int errnum) {
//        return strerror(errnum);
//    }
//    time_t mktime$UNIX2003(struct tm * a) {
//        return mktime(a);
//    }
//    double strtod$UNIX2003(const char * a, char ** b) {
//        return strtod(a, b);
//    }
//}
//#endif

//========================================================================
int main() {

	//  here are the most commonly used iOS window settings.
    //------------------------------------------------------
    ofiOSWindowSettings settings;
    settings.enableRetina = false; // enables retina resolution if the device supports it.
    settings.enableDepth = false; // enables depth buffer for 3d drawing.
    settings.enableAntiAliasing = false; // enables anti-aliasing which smooths out graphics on the screen.
    settings.numOfAntiAliasingSamples = 0; // number of samples used for anti-aliasing.
    settings.enableHardwareOrientation = true; // enables native view orientation.
    settings.enableHardwareOrientationAnimation = true; // enables native orientation changes to be animated.
    //settings.glesVersion = OFXIOS_RENDERER_ES1; // type of renderer to use, ES1, ES2, etc.
    //settings.setupOrientation = OF_ORIENTATION_90_LEFT; // set default orientation for setup

    ofCreateWindow(settings);

	ofRunApp(new ofApp);
}
