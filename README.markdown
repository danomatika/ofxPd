ofxPd: a Pure Data addon
===================================

Copyright (c) [Dan Wilcox](danomatika.com) 2011

BSD Simplified License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," in this distribution.

See https://github.com/danomatika/ofxPd for documentation as well as the [OF forum post on ofxPd](http://forum.openframeworks.cc/index.php?topic=6492.0)

DESCRIPTION
-----------

ofxPd is an Open Frameworks addon for running an instance of the Pure Data audio enviornment within an OpenFrameworks application. Audio, messages, and [MIDI](http://en.wikipedia.org/wiki/Musical_Instrument_Digital_Interface) events can be passed to and from pure data patches and the library is thread safe.

[Pure Data](http://pure-data.info/) is a graphical patching enviornment for audio and multimedia (note: the gui and graphics features are not within the scope of this addon) 

[OpenFrameworks](http://www.openframeworks.cc/) is a cross platform open source toolkit for creative coding in C++

BUILD REQUIREMENTS
------------------

To use ofxPd, first you need to download and install Open Frameworks. ofxPdExample-beta.xcodeproj is developed against the latest version of Open Frameworks on github, while ofxPdExample.xcodeproj will work with the 0062 release. ofxPdExample-ios-beta.xcodeproj is an iOS project for OF 007 which may or may not build as OF 007 is unstable at the time of this writing.

[github repository](https://github.com/openframeworks/openFrameworks)

Currently, ofxPd is being developed on Mac OSX. You will need to install Xcode from the Mac Developer Tools.

On Linux, you can use the Makefile and/or Codeblocks project files.

On Win, see the readme in the [feature-codeblocks-win branch](https://github.com/danomatika/ofxPd/tree/feature-codeblocks-win) which is currently being tested. 

BUILD AND INSTALLATION
----------------------

Place ofxPd within a folder in the apps folder of the OF dir tree:
<pre>
openframeworks/addons/ofxPd
</pre>

### Bugs

#### Undefined basic_ostream

If you get the following [linker error](http://www.openframeworks.cc/forum/viewtopic.php?f=8&t=5344&p=26537&hilit=Undefined+symbol#p26537) in xcode:
<pre>
Undefined symbols: "std::basic_ostream<char, std::char_traits<char> ...
</pre>
you need to change the Base SDK to 10.6: Project > Edit Project Settings

#### RtAudio Hang on Exit in 0062

RtAudio will hang on app exit in OF 0062. The only way to fix this is to make a small edit to the OF 0062 core by editing `lib/openFrameworks/sound/ofSoundStream.cpp` and commenting line 143 so close() is not called.

#### Distorted Sound on iOS in 007

There's a bug in the 007 release that causes disorted sound due to a typo when assigning a pointer. Edit `lib/openFrameworks/sound/ofxiPhoneSoundStream.mm` and change line 171 from
<pre>soundOutput = soundOutput;</pre>
to
<pre>soundOutputPtr = soundOutput;</pre>

Hopefully this will be fixed in the official release soon. [Fix from JonBro](https://github.com/openframeworks/openFrameworks/pull/690).

### Notes

#### Sample Rate

The sample rate is set to 44100 when initializing ofxPd in the examples. If your sample rate is higher, the playback pitch will be higher. Make sure the sample rate is the same as your system audio sample rate to hear the correct pitch.

For example: The default sample rate on Mac OSX is 96000. Running the app at 44100 results in double the playback pitch while initing ofxPd at 96000 gives the correct pitch.

### How to Create a New ofxPd Project

To develop your own project based on ofxPd, simply copy the example project and rename it. You probably want to put it in your apps folder, for example, after copying:
<pre>
openFrameworks/addons/ofxPd/example/ => openFrameworks/apps/myApps/example/
</pre>

It must be 3 levels down in the openframeworks folder structure.

Then after renaming:
<pre>
openFrameworks/apps/myApps/myPdProject/
</pre>

#### For XCode:

Rename the project in XCode (do not rename the .xcodeproj file in Finder!): XCode Menu->Project->Rename

#### For Codeblocks:

Rename the *.cbp and *workspace files to the same name as the project folder. Open the workspace, readd the renamed project file, and remove the old project.

### Adding ofxPd to an Existing Project

If you want to add ofxPd to another project, you need to make sure you include the src folder:
<pre>
openFrameworks/addons/ofxPd/src
</pre>

You will also need to include some additional C Flags for building the libpd source:

#### For XCode:

* create a new group "ofxPd" * drag these directories from ofxpd into this new group: ofxPd/src & ofxPd/libs
* add a search path to: `../../../addons/ofxPd/libs/libpd/pure-data/src` under Targets->YourApp->Build->Library Search Paths (make sure All Configurations and All Settings are selected)
* under Targets->YourApp->Build->Other C Flags (make sure All Configurations and All Settings are selected), add
	<pre>-DHAVE_UNISTD_H -DUSEAPI_DUMMY -DPD -dynamiclib -ldl -lm</pre>

#### For Linux:

* edit addons.make in your project folder and add the following line to the end of the file: 
	<pre>ofxPd</pre>
* edit config.make in your project folder and change the lines for USER_CFLAGS, USER_LDFLAGS and USER_LIBS to:
	<pre>USER_CFLAGS = -DHAVE_UNISTD_H -DUSEAPI_DUMMY -DPD -shared</pre>
	<pre>USER_LDFLAGS = --export-dynamic</pre>
	<pre>USER_LIBS = -ldl -lm</pre>

### Adding Pure Data external libraries to ofxPd

ofxPd only includes the standard set of Pure Data objects as found in the "Vanilla" version of PD. If you wish to include an external library from Pd-Extended, etc you need to include the source files in your project and call the library setup function after intiializing ofxPd in order to load the lib.

#### Adding external source files

The source files for externals included with Pd-Extended can be found in the Pure Data Subversion repository on Sourceforge. It is recommended that you use the latest Pd-Extended release branch as it will be more stable then the development version. See http://puredata.info/docs/developer/GettingPdSource

For example, if we want to include the zexy external in our project, we first download the sources files for the latest stable Pd-Extended (0.42 as of this writing) from the Subversion repository (make sure you have svn installed):
<pre>svn checkout https://pure-data.svn.sourceforge.net/svnroot/pure-data/branches/pd-extended/0.42</pre>

The external sources can be found in the `externals` folder. For instance, the zexy sources are in `externals/zexy/src/`. Copy the .h and .c files into your project folder. In my case I create an externals folder in src folder of my project, something like `myProject/src/externals/zexy`. Then add these files to your ofxPd project.

Note: Some libraries may require external libraries of their own and/or special compile time definitions. Make sure you read the build documentation on the external and include these with your project. 

Note: Some special objects included with Pd-Vanilla are not part of the libpd distribution for licensing reasons, mainly expr~, fiddle~, and sigmund~. The sources for these are found with the sources for Pd itself in the `pd/extra` folder in the Subverison repo.

#### Calling the external setup function

In order for libpd to use an external library, the library has to register itself on startup. This accomplished by calling the library's setup function which is named after the library followed by a "_setup" suffix: "library_setup()". The zexy setup function is simply "zexy_setup()". Call this setup function after initializing ofxPd in your app's setup() function:
<pre>
	if(!pd.init(numOutChannels, numInChannels, sampleRate, ticksPerBuffer)) {
		ofLog(OF_LOG_ERROR, "Could not init pd");
		OF_EXIT_APP(1);
	}
	
	// load libs
	zexy_setup();
</pre>

If all goes well, you should see some sort of print from the library as it initializes:
<pre>
[zexy] part of zexy-2.2.3 (compiled: Aug  7 2011)
	Copyright (l) 1999-2008 IOhannes m zmölnig, forum::für::umläute & IEM
[&&~] part of zexy-2.2.3 (compiled: Aug  7 2011)
	Copyright (l) 1999-2008 IOhannes m zmölnig, forum::für::umläute & IEM
[.] part of zexy-2.2.3 (compiled: Aug  7 2011)
	Copyright (l) 1999-2008 IOhannes m zmölnig, forum::für::umläute & IEM
[<~] part of zexy-2.2.3 (compiled: Aug  7 2011)
...
...
...
</pre>

For C++ and some C libraries, this is all your need. The project should compile and the external load fine. However, some pure C libraries are not written with explicit C++ support in mind and, for arcane reasons best not delved into here, the C++ compiler will not be able to find the library's setup function.  This is the case with zexy and the compiler error looks like this:
<pre>'zexy_setup' was not declared in this scope</pre>

In order for the C++ compiler to find the function, we need to add our own declaration. This can be done in your app .cpp file, a project header file, etc. In order to keep things organized, I create an "Externals.h" header file and place it in `myProject/src/externals`. Here I declare the zexy_setup function using a special syntax:
<pre>
#pragma once

extern "C" {
	void zexy_setup();
}
</pre<

The `extern "C"` keywords tell the compiler to look for a pure C function, not a C++ function. Make sure to include the "Externals.h" header file where you include "ofxPd.h". Add a setup function declaration ofr any other externals that need it here.

#### External library licensing on iOS

Apple's iOS and App Store policies forbid dynamically linking libraries. As such, you cannot include any GPL licensed externals as the GPL expressly requires dynamic linking. Submitting an app using a GPL library, such as expr~, is in violation of the GPL and will most likely result in your app being rejected from distribution in the App Store.

GPL patches, however, are not in violation of GPL distribution policies and can be included. They are not compiled into an application binary and can be replaced by the user.

DEVELOPING
----------

You can help develop ofxPd on GitHub: [https://github.com/danomatika/ofxPd](https://github.com/danomatika/ofxPd)

Create an account, clone or fork the repo, then request a push/merge.

If you find any bugs or suggestions please log them to GitHub as well.

