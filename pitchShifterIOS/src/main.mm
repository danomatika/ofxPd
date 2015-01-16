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
#include "ofMain.h"
#include "ofApp.h"

 // quick fix until 64bit OF on iOS,
// from https://github.com/openframeworks/openFrameworks/issues/3178
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR <= 8
extern "C" {
    size_t fwrite$UNIX2003(const void *a, size_t b, size_t c, FILE *d) {
        return fwrite(a, b, c, d);
    }
    char* strerror$UNIX2003(int errnum) {
        return strerror(errnum);
    }
    time_t mktime$UNIX2003(struct tm * a) {
        return mktime(a);
    }
    double strtod$UNIX2003(const char * a, char ** b) {
        return strtod(a, b);
    }
}
#endif

//========================================================================
int main() {
	ofSetupOpenGL(1024, 768, OF_FULLSCREEN);
	ofRunApp(new ofApp);
}
